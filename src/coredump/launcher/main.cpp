/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>
*/

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QPluginLoader>
#include <QProcess>
#include <QScopeGuard>
#include <QSettings>
#include <QStandardPaths>

#include <cerrno>
#include <chrono>
#include <memory>
#include <tuple>
#include <utility>

#include <poll.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>

#include "../coredump.h"
#include "../metadata.h"
#include "../socket.h"
#include "DumpTruckInterface.h"

using namespace std::chrono_literals;

static QString drkonqiExe()
{
    // Borrowed from kcrash.cpp
    static QStringList paths = QFile::decodeName(qgetenv("LIBEXEC_PATH")).split(QLatin1Char(':'), Qt::SkipEmptyParts)
        + QStringList{
            QCoreApplication::applicationDirPath(), // then look where our application binary is located
            QLibraryInfo::location(QLibraryInfo::LibraryExecutablesPath), // look where libexec path is (can be set in qt.conf)
            QFile::decodeName(KDE_INSTALL_FULL_LIBEXECDIR), // look at our installation location
        };
    static QString exec = QStandardPaths::findExecutable(QStringLiteral("drkonqi"), paths);
    return exec;
}

using ArgumentsPidTuple = std::tuple<QStringList, bool>;

static ArgumentsPidTuple metadataArguments(const Coredump &dump, const QString &metadataPath)
{
    QStringList arguments;
    bool foundPID = false;

    // Parse the metadata file. Ideally we'd should even stop passing a gazillion options
    // and instead rely on this file, then drkonqi
    // would also be in charge of removing it instead of us here.
    QSettings metadata(metadataPath, QSettings::IniFormat);
    metadata.beginGroup(QStringLiteral("KCrash"));
    const QStringList keys = metadata.allKeys();
    for (const QString &key : keys) {
        const QString value = metadata.value(key).toString();

        if (key == QLatin1String("exe")) {
            if (value.endsWith(QStringLiteral("/drkonqi"))) {
                qWarning() << "drkonqi crashed, we aren't going to invoke it again, we might be the reason it crashd :O";
                return {};
            }
            if (value != dump.exe) {
                qWarning() << "the exe in the metadata file doesn't match the exe in the journal entry! aborting" << value << dump.exe;
                return {};
            }
            // exe purely exists for our benefit, don't forward it to drkonqi.
            continue;
        }

        if (key == QLatin1String("pid")) {
            foundPID = true;
        }

        arguments << QStringLiteral("--%1").arg(key);
        if (value != QLatin1String("true") && value != QLatin1String("false")) { // not a bool value, append as arg
            arguments << value;
        }
    }
    metadata.endGroup();

    return {arguments, foundPID};
}

static ArgumentsPidTuple includeAllArguments(const Coredump &dump)
{
    // Synthesize drkonqi arguments from the dump alone, this has limited functionality and is meant for use with
    // non-KDE apps.
    QStringList arguments;
    arguments << QStringLiteral("--signal") << QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_SIGNAL")));
    arguments << QStringLiteral("--pid") << QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_PID")));
    arguments << QStringLiteral("--restarted"); // Cannot restart foreign apps!
    return {arguments, true};
}

static ArgumentsPidTuple includeKWinWaylandArguments(const Coredump &dump)
{
    auto [arguments, foundPID] = includeAllArguments(dump);
    arguments << QStringLiteral("--bugaddress") << QStringLiteral("submit@bugs.kde.org") << QStringLiteral("--appname") << dump.exe;
    return {arguments, foundPID};
}

static bool tryDrkonqi(const Coredump &dump)
{
    const QString metadataPath = Metadata::resolveMetadataPath(dump.pid);
    const QString configFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("drkonqirc"));

    // Arm removal. If we return early this will ensure clean up, otherwise we dismiss it later
    auto deleteFile = qScopeGuard([metadataPath] {
        QFile::remove(metadataPath);
    });

    if (qEnvironmentVariableIsSet("KDE_DEBUG")) {
        qWarning() << "KDE_DEBUG set. Not invoking DrKonqi.";
        return false;
    }

    if (drkonqiExe().isEmpty()) {
        qWarning() << "Couldn't find drkonqi exe";
        return false;
    }

    QStringList arguments;
    bool foundPID = false;

    if (!metadataPath.isEmpty()) {
        // A KDE app crash has metadata, build arguments from that
        std::tie(arguments, foundPID) = metadataArguments(dump, metadataPath);
    } else if (dump.exe.endsWith(QLatin1String("/kwin_wayland"))) {
        // When the compositor goes down it may not have time to store metadata, in that case we'll fake them.
        std::tie(arguments, foundPID) = includeKWinWaylandArguments(dump);
    } else if (QSettings(configFile, QSettings::IniFormat).value(QStringLiteral("IncludeAll")).toBool()) {
        // Handle non-KDE apps when in IncludeAll mode.
        std::tie(arguments, foundPID) = includeAllArguments(dump);
    } else {
        return false;
    }

    if (arguments.isEmpty() || !foundPID) {
        // There is a chance that somehow the metadata file writing failed or is incomplete. Do some trivial
        // checks to catch and ignore such cases. Otherwise we risk drkonqi crashing due to internally failed
        // assertions.
        qWarning() << "Failed to read metadata from crash" << arguments << foundPID;
        return false;
    }

    // Append Coredump data. This allow us to not have to talk to journald again on the drkonqi side.
    QSettings metadata(Metadata::metadataPath(dump.pid), QSettings::IniFormat);
    metadata.beginGroup(QStringLiteral("Journal"));
    for (auto it = dump.m_rawData.cbegin(); it != dump.m_rawData.cend(); ++it) {
        metadata.setValue(QString::fromUtf8(it.key()), it.value());
    }
    metadata.endGroup();
    metadata.sync();

    setenv("DRKONQI_BACKEND", "COREDUMPD", 1);
    setenv("DRKONQI_METADATA_FILE", qPrintable(metadataPath), 1);

    deleteFile.dismiss(); // let drkonqi handle cleanup if we get here

    // We must start drkonqi in a new slice. This launcher will want to terminate quickly and we enforce that
    // through maximum run time in the unit configuration. If drkonqi wasn't in a new slice it'd get killed with us.
    QProcess::execute(drkonqiExe(), arguments);

    return true; // always considered handled, even if drkonqi crashes or something
}

class DrKonqiTruck : public DumpTruckInterface
{
public:
    DrKonqiTruck() = default;
    ~DrKonqiTruck() override = default;

    bool handle(const Coredump &dump) override
    {
        if (!QFile::exists(dump.filename)) {
            QFile::remove(Metadata::resolveMetadataPath(dump.pid)); // without trace we'll never run drkonqi as it can't file a bug anyway
            return false;
        }

        return tryDrkonqi(dump);
    }

private:
    Q_DISABLE_COPY_MOVE(DrKonqiTruck)
};

static void onNewDump(const Coredump &dump)
{
    static DrKonqiTruck drkonqi;
    if (drkonqi.handle(dump)) {
        return;
    }

    if (qEnvironmentVariableIntValue("KDE_COREDUMP_NOTIFY") >= 1) {
        // Developers need to explicitly opt into the notifications. They
        // have no l10n and are also uniquely useless to users.
        static QPluginLoader loader(QStringLiteral("drkonqi/KDECoredumpNotifierTruck"));
        if (!loader.load()) {
            qWarning() << "failed to load" << loader.fileName() << loader.errorString();
            return;
        }
        auto notifier = qobject_cast<DumpTruckInterface *>(loader.instance());
        Q_ASSERT(notifier);
        if (notifier->handle(dump)) {
            return;
        }
    }

    qWarning() << "Nothing handled the dump :O";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("drkonqi-coredump-launcher"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    if (sd_listen_fds(false) != 1) {
        qFatal("Not exactly one fd passed by systemd. Quel malheur!");
        return 1;
    }

    // Reading is very awkward and I'm not even sure why.
    // It seems like QLocalSocket doesn't manage to model stream sockets properly or at least not SOCK_SEQPACKET.
    // The socket on our end never notices that the remote has closed and even terminated already.
    //
    // Since we don't really need to do anything fancy we'll simply poll on our own instead of relying on QLS.

    QByteArray json;
    QByteArray segment;
    segment.resize(Socket::DatagramSize);
    while (true) {
        struct pollfd poll {
        };
        poll.fd = SD_LISTEN_FDS_START;
        poll.events = POLLIN;
        struct timespec time {
        };
        time.tv_nsec = (2500ns).count();
        ppoll(&poll, 1, &time, nullptr);

        if (poll.revents & POLLERR) {
            qFatal("Socket had an error");
            break;
        }

        if (poll.revents & POLLNVAL) {
            qFatal("Socket was invalid");
            break;
        }

        if (poll.revents & POLLIN) {
            const size_t size = read(poll.fd, segment.data(), segment.size());
            if (size == 0 && poll.revents & POLLHUP) {
                break; // zero read + POLLHUP = EOS says the manpage
            }
            json.append(segment.data(), size);
        }
    }
    close(SD_LISTEN_FDS_START);

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "json parse error" << error.errorString();
        return 1;
    }

    // Unset a slew of systemd variables.
    unsetenv("JOURNAL_STREAM");
    unsetenv("INVOCATION_ID");
    unsetenv("LISTEN_FDNAMES");
    unsetenv("LISTEN_FDS");
    unsetenv("LISTEN_PID");
    unsetenv("MANAGERPID");

    onNewDump(Coredump(document));

    return 0;
}
