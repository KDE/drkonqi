/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
*/

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QScopeGuard>
#include <QSocketNotifier>

#include <cerrno>
#include <memory>
#include <optional>
#include <utility>

#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <systemd/sd-journal.h>
#include <unistd.h>

#include "../coredump.h"
#include "../socket.h"

namespace std
{
template<>
struct default_delete<sd_journal> {
    void operator()(sd_journal *ptr) const
    {
        sd_journal_close(ptr);
    }
};
} // namespace std

template<typename T>
struct Expected {
    const int ret; // return value of call
    const int error; // errno immediately after the call
    std::unique_ptr<T> value; // the newly owned object (may be null)
};

// Wrapper around C double pointer API of which we must take ownership.
// errno may or may not be
template<typename T, typename Func, typename... Args>
Expected<T> owning_ptr_call(Func func, Args &&...args)
{
    T *raw = nullptr;
    const int ret = func(&raw, std::forward<Args>(args)...);
    return {ret, errno, std::unique_ptr<T>(raw)};
}

// Same as owning_ptr_call but for (sd_journal *, foo **, ...) API
template<typename T, typename Func, typename... Args>
Expected<T> contextual_owning_ptr_call(Func func, sd_journal *context, Args &&...args)
{
    T *raw = nullptr;
    const int ret = func(context, &raw, std::forward<Args>(args)...);
    return {ret, errno, std::unique_ptr<T>(raw)};
}

class CoredumpWatcher : public QObject
{
    Q_OBJECT
public:
    explicit CoredumpWatcher(std::unique_ptr<sd_journal> context_, QString bootId_, const QString &instance_, QObject *parent = nullptr)
        : QObject(parent)
        , context(std::move(context_))
        , bootId(std::move(bootId_))
        , instance(instance_)
        , instanceFilter(QStringLiteral("systemd-coredump@%1").arg(instance_))
    {
    }

    void start()
    {
        Q_ASSERT(context);

        sd_journal_flush_matches(context.get()); // reset match
        if (sd_journal_add_match(context.get(), "SYSLOG_IDENTIFIER=systemd-coredump", 0) != 0) {
            Q_EMIT error(QStringLiteral("Failed to install id match"));
            return;
        }

        const QString bootIdMatch = QStringLiteral("_BOOT_ID=%1").arg(bootId);
        if (sd_journal_add_match(context.get(), qPrintable(bootIdMatch), 0) != 0) {
            Q_EMIT error(QStringLiteral("Failed to install boot id match"));
            return;
        }

        if (instance.count(QLatin1Char('-')) >= 3) {
            // older systemds have a bug where %I doesn't actually expand correctly and only contains the first element.
            // This prevents us from matching through sd API. Instead processLog will filter based on the instance
            // information prefix. It's still unique enough.
            // Auto-generated instance names are of the form
            // $iid-$pid-$uid where iid is a growing instance id number.
            // Additionally we'll filter by chrono proximity, iids that are too far in the past will be discarded.
            // This is because iid on its own isn't necessarily unique in the event that it wraps around whatever
            // integer limit it has.
            if (sd_journal_add_match(context.get(), qPrintable(QStringLiteral("_SYSTEMD_UNIT=%1").arg(instanceFilter)), 0) != 0) {
                Q_EMIT error(QStringLiteral("Failed to install unit match"));
                return;
            }
        }

        const int fd = sd_journal_get_fd(context.get());
        if (fd < 0) {
            errnoError(QStringLiteral("Failed to get listening socket"), -fd);
            return;
        }

        notifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read);
        connect(notifier.get(), &QSocketNotifier::activated, this, [this] {
            if (sd_journal_process(context.get()) != SD_JOURNAL_APPEND) {
                return;
            }
            processLog();
        });

        if (int ret = sd_journal_seek_head(context.get()); ret != 0) {
            errnoError(QStringLiteral("Failed to go to tail"), -fd);
            return;
        }
        // Make sure to read whatever we have pending on next loop.
        QMetaObject::invokeMethod(this, &CoredumpWatcher::processLog);
    }

Q_SIGNALS:
    void finished();
    void error(const QString &msg);

private:
    void processLog();
    void errnoError(const QString &msg, int err)
    {
        Q_EMIT error(msg + QStringLiteral(": (%1) ").arg(QString::number(err)) + QString::fromLocal8Bit(strerror(err)));
    }

    const std::unique_ptr<sd_journal> context = nullptr;
    std::unique_ptr<QSocketNotifier> notifier = nullptr;
    const QString bootId;
    const QString instance;
    const QString instanceFilter; // systemd-coredump@%1 instance name
};

static std::optional<Coredump> makeDump(sd_journal *context)
{
    auto cursorExpected = contextual_owning_ptr_call<char>(sd_journal_get_cursor, context);
    if (cursorExpected.ret != 0) {
        qFatal("Failed to get entry cursor");
        return std::nullopt;
    }

    Coredump::EntriesHash entries;
    const void *data = nullptr;
    size_t length = 0;
    SD_JOURNAL_FOREACH_DATA(context, data, length)
    {
        // size_t is uint, QBA uses int, make sure we don't overflow the int!
        int dataSize = static_cast<int>(length);
        Q_ASSERT(dataSize >= 0);
        Q_ASSERT(static_cast<quint64>(dataSize) == length);

        QByteArray entry(static_cast<const char *>(data), dataSize);
        const int offset = entry.indexOf('=');
        if (offset < 0) {
            qWarning() << "this entry looks funny it has no separating = character" << entry;
            continue;
        }

        const QByteArray key = entry.left(offset);
        if (key == QByteArrayLiteral("COREDUMP")) {
            // The literal COREDUMP= entry is the actual core when configured for journal storage in coredump.conf.
            // Synthesize a filename instead so we can use the same validity checks for all storage types.
            entries.insert(Coredump::keyFilename(), QByteArrayLiteral("/dev/null"));
            continue;
        }

        const QByteArray value = entry.mid(offset + 1);

        // Always add to raw data, they get serialized back into the INI file for drkonqi.
        entries.insert(key, value);
    }

    return std::make_optional<Coredump>(cursorExpected.value.get(), entries);
}

void CoredumpWatcher::processLog()
{
    while (sd_journal_next(context.get()) > 0) {
        const auto optionalDump = makeDump(context.get());
        if (!optionalDump.has_value()) {
            qWarning() << "Failed to make a dump :O";
            continue;
        }

        const Coredump &dump = optionalDump.value();
        if (!dump.systemd_unit.startsWith(instanceFilter)) {
            // Older systemds have trouble templating a correct instance. We only
            // perform a startsWith check here, but will filter more aggressively
            // whenever possible via the constructor.
            continue;
        }
        if (dump.exe.isEmpty() && dump.filename.isEmpty()) {
            qDebug() << "Entry doesn't look like a dump. This may have been a vaccum run. Nothing to process.";
            // Do not finish here. Vaccum log entires are created from real coredump processes. We should eventually
            // find a dump.
            continue;
        }

        qDebug() << dump.exe << dump.pid << dump.filename;

        // We only try to find the socket file here because we need to know the UID and on older systemd's we'll not
        // be able to figure this out from just the instance information.
        // When systemd 245 (Ubuntu 20.04) no longer is out in the wild we can move this into the main and get the
        // uid from the instance.
        const QByteArray socketPath = QByteArrayLiteral("/run/user/") + QByteArray::number(dump.uid) + QByteArrayLiteral("/drkonqi-coredump-launcher");
        if (!QFile::exists(QString::fromUtf8(socketPath))) {
            // This is intentionally not an error or fatal, not all users necessarily have
            // a socket or drkonqi!
            qWarning() << "The socket path doesn't exist @" << socketPath;
            Q_EMIT finished();
            return;
        }

        sockaddr_un sa{};
        sa.sun_family = AF_UNIX;
        // size_t is signed, ensure path is too
        Q_ASSERT(socketPath.size() >= 0);
        const std::make_unsigned<decltype(socketPath.size())>::type pathSize = socketPath.size();
        if (pathSize > sizeof(sa.sun_path) /* '>' because we need an extra byte for null */) {
            Q_EMIT error(QStringLiteral("The socket path has too many characters:") + QString::fromLatin1(socketPath));
            return;
        }
        strncpy(static_cast<char *>(sa.sun_path), socketPath.constData(), sizeof(sa.sun_path));

        const int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            Q_EMIT error(QString::fromLocal8Bit(strerror(errno)));
            Q_UNREACHABLE();
        }
        QScopeGuard closeFD([fd] {
            close(fd);
        });

        if (::connect(fd, (sockaddr *)&sa, sizeof(sa)) < 0) { // NOLINT
            Q_EMIT error(QString::fromLocal8Bit(strerror(errno)));
            return;
        }

        // Convert the raw data to JSON and send that over the socket. This means
        // the client side doesn't need to talk to journald again. A tad more efficient,
        // and it makes nary a difference in code.
        QVariantMap variantMap;
        for (auto it = dump.m_rawData.cbegin(); it != dump.m_rawData.cend(); ++it) {
            variantMap.insert(QString::fromUtf8(it.key()), it.value());
        }

        QLocalSocket s;
        s.setSocketDescriptor(fd, QLocalSocket::ConnectedState, QLocalSocket::WriteOnly);
        QByteArray data = QJsonDocument::fromVariant(variantMap).toJson();
        while (!data.isEmpty()) {
            // NB: we need to constrain the segment size to not run into QLocalSocket::DatagramTooLargeError
            const qint64 written = s.write(data.constData(), std::min<int>(data.size(), Socket::DatagramSize));
            if (written > 0) {
                data = data.mid(written);
                s.waitForBytesWritten();
            }
        }
        s.flush();
        s.waitForBytesWritten();

        Q_EMIT finished();
        return;
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("drkonqi-coredump-helper"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    // This binary is for internal use and intentionally has no i18n!
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("boot-id"), QStringLiteral("Boot ID"));
    parser.addPositionalArgument(QStringLiteral("instance"), QStringLiteral("Template instance (forwareded from systemd-coredump@)"));
    parser.process(app);
    const QStringList args = parser.positionalArguments();
    Q_ASSERT(args.size() == 2);
    const QString &bootId = args.at(0);
    const QString &instance = args.at(1);

    auto expectedJournal = owning_ptr_call<sd_journal>(sd_journal_open, SD_JOURNAL_LOCAL_ONLY);
    Q_ASSERT(expectedJournal.ret == 0);
    Q_ASSERT(expectedJournal.value);

    CoredumpWatcher w(std::move(expectedJournal.value), bootId, instance, nullptr);
    QObject::connect(&w, &CoredumpWatcher::finished, &app, &QCoreApplication::quit, Qt::QueuedConnection);
    QObject::connect(
        &w,
        &CoredumpWatcher::error,
        &app,
        [](const QString &msg) {
            qWarning() << msg;
            qApp->exit(1);
        },
        Qt::QueuedConnection);
    w.start();
    return app.exec();
}

#include "main.moc"
