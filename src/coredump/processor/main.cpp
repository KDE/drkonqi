/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2022 Harald Sitter <sitter@kde.org>
*/

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QScopeGuard>

#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <coredump.h>
#include <coredumpwatcher.h>
#include <socket.h>

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

    CoredumpWatcher watcher(std::move(expectedJournal.value), bootId, instance, nullptr);
    QObject::connect(&watcher, &CoredumpWatcher::newDump, &app, [&watcher](const Coredump &dump) {
        // We only try to find the socket file here because we need to know the UID and on older systemd's we'll not
        // be able to figure this out from just the instance information.
        // When systemd 245 (Ubuntu 20.04) no longer is out in the wild we can move this into the main and get the
        // uid from the instance.
        const QByteArray socketPath = QByteArrayLiteral("/run/user/") + QByteArray::number(dump.uid) + QByteArrayLiteral("/drkonqi-coredump-launcher");
        if (!QFile::exists(QString::fromUtf8(socketPath))) {
            // This is intentionally not an error or fatal, not all users necessarily have
            // a socket or drkonqi!
            qWarning() << "The socket path doesn't exist @" << socketPath;
            Q_EMIT watcher.finished();
            return;
        }

        sockaddr_un sa{};
        sa.sun_family = AF_UNIX;
        // size_t is signed, ensure path is too
        Q_ASSERT(socketPath.size() >= 0);
        const std::make_unsigned<decltype(socketPath.size())>::type pathSize = socketPath.size();
        if (pathSize > sizeof(sa.sun_path) /* '>' because we need an extra byte for null */) {
            Q_EMIT watcher.error(QStringLiteral("The socket path has too many characters:") + QString::fromLatin1(socketPath));
            return;
        }
        strncpy(static_cast<char *>(sa.sun_path), socketPath.constData(), sizeof(sa.sun_path));

        const int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            Q_EMIT watcher.error(QString::fromLocal8Bit(strerror(errno)));
            Q_UNREACHABLE();
        }
        QScopeGuard closeFD([fd] {
            close(fd);
        });

        if (::connect(fd, (sockaddr *)&sa, sizeof(sa)) < 0) { // NOLINT
            Q_EMIT watcher.error(QString::fromLocal8Bit(strerror(errno)));
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

        Q_EMIT watcher.finished();
        return;
    });

    QObject::connect(&watcher, &CoredumpWatcher::finished, &app, &QCoreApplication::quit, Qt::QueuedConnection);
    QObject::connect(
        &watcher,
        &CoredumpWatcher::error,
        &app,
        [](const QString &msg) {
            qWarning() << msg;
            qApp->exit(1);
        },
        Qt::QueuedConnection);
    watcher.start();

    return app.exec();
}
