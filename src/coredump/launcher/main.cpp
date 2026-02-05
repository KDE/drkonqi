/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>
*/

#include <poll.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>

#include <chrono>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QProcess>
#include <QScopeGuard>
#include <QStandardPaths>

#include <KConfig>
#include <KConfigGroup>
#include <kcrash_version.h>

#include <drkonqipaths.h>
#include <metadata.h>

#include "../coredump.h"
#include "../socket.h"
#include "DevNotifierTruck.h"
#include "DumpTruckInterface.h"
#include "GlobalNotifierTruck.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

QJsonObject jsonObjectFromKConfigGroup(const KConfigGroup &group)
{
    QJsonObject object;
    const QStringList keys = group.keyList();
    for (const auto &key : keys) {
        const auto value = group.readEntry(key);
        object.insert(key, value);
    }
    return object;
}

[[nodiscard]] QJsonObject kcrashToDrKonqiMetadata(const Coredump &dump, const QString &kcrashMetadataPath)
{
    auto contextObject = QJsonObject{{u"version"_s, 2}};
    KConfig kcrashMetadata(kcrashMetadataPath, KConfig::SimpleConfig);
    {
        contextObject.insert(Metadata::KCRASH_KEY, jsonObjectFromKConfigGroup(kcrashMetadata.group(u"KCrash"_s)));
        contextObject.insert(Metadata::KCRASH_TAGS_KEY, jsonObjectFromKConfigGroup(kcrashMetadata.group(u"KCrashTags"_s)));
        contextObject.insert(Metadata::KCRASH_EXTRA_DATA_KEY, jsonObjectFromKConfigGroup(kcrashMetadata.group(u"KCrashExtra"_s)));
        contextObject.insert(Metadata::KCRASH_GPU_KEY, jsonObjectFromKConfigGroup(kcrashMetadata.group(u"KCrashGPU"_s)));
        contextObject.insert(Metadata::KCRASH_COMPLETE_KEY, kcrashMetadata.hasGroup(u"KCrashComplete"_s));
    }
    {
        QJsonObject journalObject;
        for (auto it = dump.m_rawData.cbegin(); it != dump.m_rawData.cend(); ++it) {
            journalObject.insert(QString::fromUtf8(it.key()), QString::fromUtf8(it.value()));
        }
        contextObject.insert(u"journal"_s, journalObject);
    }
    {
        contextObject.insert(Metadata::DRKONQI_KEY, QJsonObject{{Metadata::PICKED_UP_KEY, true}});
    }
    if (kcrashMetadata.hasGroup(u"KCrash"_s) && !kcrashMetadata.hasGroup(u"KCrashComplete"_s)) {
        // introduced in KCrash 6.23 to mark that the file was fully written (i.e. no crash mid-write).
        // Fill a bunch of fallbacks when not complete. This will prevent drkonqi from failing assertions about missing data.
        auto kcrash = contextObject[Metadata::KCRASH_KEY].toObject();
        auto containsAndValid = [&kcrash](const QString &key) {
            return kcrash.contains(key) && !kcrash.value(key).isUndefined();
        };
        if (!containsAndValid("signal"_L1)) {
            kcrash.insert(u"signal"_s, QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_SIGNAL"))));
        }
        if (!containsAndValid("pid"_L1)) {
            kcrash.insert(u"pid"_s, QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_PID"))));
        }
        if (!containsAndValid("restarted"_L1)) {
            kcrash.insert(u"restarted"_s, true); // Unknown; pretend it was restarted lest we restart twice.
        }
        contextObject[Metadata::KCRASH_KEY] = kcrash;
    } else {
#if KCRASH_VERSION >= QT_VERSION_CHECK(6, 23, 0)
        qWarning() << "KCrash metadata not marked complete. Please file a bug at bugs.kde.org giving as much detail about the crash as possible and maybe "
                      "include the file"
                   << kcrashMetadataPath;
#endif
    }

    return contextObject;
}

[[nodiscard]] QJsonObject &synthesizeKCrashInto(const Coredump &dump, QJsonObject &metadata)
{
    if (!metadata[Metadata::KCRASH_KEY].toObject().isEmpty()) {
        return metadata; // already has data
    }
    if (!dump.exe.endsWith("/kwin_wayland"_L1) && !dump.exe.endsWith("/kwin_x11"_L1)) {
        return metadata; // isn't kwin
    }

    auto object = metadata[Metadata::KCRASH_KEY].toObject();
    object.insert(u"signal"_s, QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_SIGNAL"))));
    object.insert(u"pid"_s, QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_PID"))));
    object.insert(u"restarted"_s, true); // Cannot restart kwin. Autostarts if anything.
    object.insert(u"bugaddress"_s, u"submit@bugs.kde.org"_s);
    object.insert(u"appname"_s, dump.exe);
    metadata[Metadata::KCRASH_KEY] = object;

    return metadata;
}

[[nodiscard]] QJsonObject &synthesizeGenericInto(const Coredump &dump, QJsonObject &metadata)
{
    auto object = metadata[Metadata::KCRASH_KEY].toObject();

    if (!object.isEmpty()) {
        return metadata;
    }

    static const QString configFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("drkonqirc"));
    if (!KConfig(configFile, KConfig::SimpleConfig).group(u"General"_s).readEntry(QStringLiteral("IncludeAll"), false)) {
        return metadata;
    }

    object.insert(u"signal"_s, QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_SIGNAL"))));
    object.insert(u"pid"_s, QString::fromUtf8(dump.m_rawData.value(QByteArrayLiteral("COREDUMP_PID"))));
    object.insert(u"restarted"_s, true); // Cannot restart foreign apps!
    metadata[Metadata::KCRASH_KEY] = object;

    return metadata;
}

void writeToDisk(const QJsonObject &contextObject, const QString &drkonqiMetadataPath)
{
    QDir().mkpath(QFileInfo(drkonqiMetadataPath).path());

    QFile file(drkonqiMetadataPath);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        file.write(QJsonDocument(contextObject).toJson());
    } else {
        qWarning() << "Failed to open for writing:" << drkonqiMetadataPath;
    }
}

static bool tryDrkonqi(const Coredump &dump)
{
    const QString kcrashMetadataPath = Metadata::resolveKCrashMetadataPath(dump.exe, dump.bootId, dump.pid);
    // Arm removal. In all cases we'll want to remove the kcrash metadata (we possibly created expanded drkonqi metadata instead)
    auto deleteFile = qScopeGuard([kcrashMetadataPath] {
        if (!kcrashMetadataPath.isEmpty()) { // don't warn about null path
            QFile::remove(kcrashMetadataPath);
        }
    });

#if !defined(WITH_DRKONQI_REPORTING)
    return false; // DrKonqi reporting disabled at compile time. Let this crash fall through to the global notifier.
#endif

    const QString drkonqiMetadataPath = Metadata::drkonqiMetadataPath(dump.exe, dump.bootId, dump.timestamp, dump.pid);

    QJsonObject metadata = Metadata::readFromDisk(drkonqiMetadataPath);
    if (Metadata::isPickedUp(metadata)) {
        return true; // already handled previously
    }
    if (metadata.isEmpty()) {
        // if our metadata doesn't exist yet try to pick it up from kcrash
        metadata = kcrashToDrKonqiMetadata(dump, kcrashMetadataPath);
        // or synthesize it
        metadata = synthesizeKCrashInto(dump, metadata);
        // or force-handle the crash
        metadata = synthesizeGenericInto(dump, metadata);
        writeToDisk(metadata, drkonqiMetadataPath);
    }

    if (metadata.isEmpty() || metadata[Metadata::KCRASH_KEY].toObject().isEmpty()) {
        return false; // no metadata, or no kcrash metadata -> don't know what to do with this
    }

    if (!QFile::exists(dump.filename)) {
        return false; // no trace -> nothing to handle
    }

    if (qEnvironmentVariableIsSet("KDE_DEBUG")) {
        qWarning() << "KDE_DEBUG set. Not invoking DrKonqi.";
        return false;
    }

    if (Paths::drkonqiExe().isEmpty()) {
        qWarning() << "Couldn't find drkonqi exe";
        return false;
    }

    setenv("DRKONQI_BACKEND", "COREDUMPD", 1);
    setenv("DRKONQI_METADATA_FILE", qPrintable(drkonqiMetadataPath), 1);

    // We must start drkonqi in a new slice. This launcher will want to terminate quickly and we enforce that
    // through maximum run time in the unit configuration. If drkonqi wasn't in a new slice it'd get killed with us.
    QProcess::execute(Paths::drkonqiExe(), Metadata::metadataArguments(metadata[Metadata::KCRASH_KEY].toObject().toVariantHash()));

    return true; // always considered handled, even if drkonqi crashes or something
}

class DrKonqiTruck : public DumpTruckInterface
{
public:
    DrKonqiTruck() = default;
    ~DrKonqiTruck() override = default;

    bool handle(const Coredump &dump) override
    {
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

    if (qEnvironmentVariableIntValue("KDE_COREDUMP_NOTIFY") == 1) {
        static DevNotifierTruck notifier;
        if (notifier.handle(dump)) {
            return;
        }
    }

    static GlobalNotifierTruck notifier;
    if (notifier.handle(dump)) {
        return;
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
