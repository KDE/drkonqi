// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <filesystem>
#include <optional>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusMetaType>
#include <QDBusUnixFileDescriptor>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>

#include <polkitqt1-agent-session.h>
#include <polkitqt1-authority.h>

#include <coredumpexcavator.h>

using namespace Qt::StringLiterals;

namespace
{
constexpr auto COREDUMP_PATH = "/var/lib/systemd/coredump/"_L1; // The path is hardcoded in systemd's coredump.c
constexpr auto ACTION_NAME = "org.kde.drkonqi.saveCoreToDir"_L1;
constexpr auto CORE_NAME = "core"_L1;

using ErrorString = QString;
std::optional<ErrorString> renameOrCopy(int sourceDirFd, int targetDirFd)
{
    if (renameat(sourceDirFd, CORE_NAME.latin1(), targetDirFd, CORE_NAME.latin1()) == 0) {
        return std::nullopt;
    }
    if (errno != EXDEV) {
        auto err = errno;
        return u"Failed to rename between directory fds: %1"_s.arg(QString::fromUtf8(strerror(err)));
    }

    const auto inFd = openat(sourceDirFd, CORE_NAME.latin1(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (inFd == -1) {
        auto err = errno;
        return u"Failed to open input file: %1"_s.arg(QString::fromUtf8(strerror(err)));
    }
    auto closeInFd = qScopeGuard([inFd] {
        close(inFd);
    });

    const auto outFd = openat(targetDirFd, CORE_NAME.latin1(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_CREAT | O_TRUNC);
    if (outFd == -1) {
        auto err = errno;
        return u"Failed to open output file: %1"_s.arg(QString::fromUtf8(strerror(err)));
    }
    auto closeOutFd = qScopeGuard([outFd] {
        close(outFd);
    });

    // Try copy_file_range
    ssize_t ret = 0;
    while ((ret = copy_file_range(inFd, nullptr, outFd, nullptr, 128 * 1024 * 1024 /* MiB */, 0)) > 0) { }
    if (ret == 0) {
        return {};
    }

    auto err = errno;
    return u"Failed copy_file_range: %1"_s.arg(QString::fromUtf8(strerror(err)));
}

} // namespace

class Helper : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.drkonqi")

public Q_SLOTS:
    QString saveCoreToDir(const QString &coreName, const QDBusUnixFileDescriptor &targetDirFd)
    {
        auto loopLock = std::make_shared<QEventLoopLocker>();

        if (coreName.contains('/'_L1)) { // don't allow anything that could look like a path
            sendErrorReply(QDBusError::AccessDenied);
            return {};
        }

        // Create the tmpdir early because we need to know the involved paths ahead of authorization since we forward
        // them into the polkit auth agent UI.
        QTemporaryDir tmpDir(QDir::tempPath() + "/drkonqi-coredump-excavator"_L1);
        if (!tmpDir.isValid()) {
            qWarning() << "tmpdir not valid";
            sendErrorReply(QDBusError::InternalError, "Failed to create temporary directory"_L1);
            return {};
        }

        // Ensure the core is kept secure!
        Q_ASSERT([&tmpDir]() -> bool {
            const auto targetDir = tmpDir.path();
            const auto permissions = std::filesystem::status(targetDir.toStdString()).permissions();
            return permissions == std::filesystem::perms::owner_all;
        }());

        const QString coreFileDir = tmpDir.path();
        const QString coreFileTarget = tmpDir.filePath(CORE_NAME);
        const QString coreFile = COREDUMP_PATH + coreName;

        if (!isAuthorized(coreFile, coreFileTarget)) {
            qWarning() << "not authorized";
            sendErrorReply(QDBusError::AccessDenied);
            return {};
        }

        setDelayedReply(true);

        auto connection = this->connection();
        auto msg = message();

        auto excavator = new CoredumpExcavator(this);
        connect(excavator,
                &CoredumpExcavator::excavated,
                this,
                [msg, tmpDir = std::move(tmpDir), loopLock, connection, excavator, coreFileDir, coreFileTarget, targetDirFd](int exitCode) mutable {
                    excavator->deleteLater();

                    if (exitCode != 0) {
                        connection.send(msg.createReply() << QString());
                        return;
                    }

                    int sourceDirFd = open(qUtf8Printable(coreFileDir), O_RDONLY | O_CLOEXEC | O_DIRECTORY);
                    if (sourceDirFd < 0) {
                        int err = errno;
                        QString errString = u"Failed to open coreFileDir(%1): %2"_s.arg(coreFileDir, QString::fromUtf8(strerror(err)));
                        connection.send(msg.createErrorReply(QDBusError::InternalError, errString));
                        return;
                    }
                    auto closeFd = qScopeGuard([sourceDirFd] {
                        close(sourceDirFd);
                    });

                    if (auto errorString = renameOrCopy(sourceDirFd, targetDirFd.fileDescriptor()); errorString.has_value()) {
                        connection.send(msg.createErrorReply(QDBusError::InternalError, errorString.value()));
                        return;
                    }

                    connection.send(msg.createReply() << CORE_NAME);
                });

        excavator->excavateFromTo(coreFile, coreFileTarget);

        return {};
    }

private:
    bool isAuthorized(const QString &coreName, const QString &coreFileTarget)
    {
        auto authority = PolkitQt1::Authority::instance();
        auto result = authority->checkAuthorizationSyncWithDetails(ACTION_NAME,
                                                                   PolkitQt1::SystemBusNameSubject(message().service()),
                                                                   PolkitQt1::Authority::AllowUserInteraction,
                                                                   {
                                                                       {"coreFile"_L1, coreName}, //
                                                                       {"coreFileTarget"_L1, coreFileTarget}, //
                                                                   });

        if (authority->hasError()) {
            qWarning() << authority->lastError() << authority->errorDetails();
            authority->clearError();
            return false;
        }

        switch (result) {
        case PolkitQt1::Authority::Yes:
            return true;
        case PolkitQt1::Authority::Unknown:
        case PolkitQt1::Authority::No:
        case PolkitQt1::Authority::Challenge:
            break;
        }
        return false;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(true);

    Helper helper;

    if (!QDBusConnection::systemBus().registerObject(QStringLiteral("/"), &helper, QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register the daemon object" << QDBusConnection::systemBus().lastError().message();
        return 1;
    }
    if (!QDBusConnection::systemBus().registerService(QStringLiteral("org.kde.drkonqi"))) {
        qWarning() << "Failed to register the service" << QDBusConnection::systemBus().lastError().message();
        return 1;
    }

    return app.exec();
}

#include "main.moc"
