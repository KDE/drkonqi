// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include <fcntl.h>

#include <filesystem>

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

class Helper : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.drkonqi")

    static constexpr auto COREDUMP_PATH = "/var/lib/systemd/coredump/"_L1; // The path is hardcoded in systemd's coredump.c
    static constexpr auto ACTION_NAME = "org.kde.drkonqi.excavateFromToDirFd"_L1;
    static constexpr auto CORE_NAME = "core"_L1;

public Q_SLOTS:
    QString excavateFromToDirFd(const QString &coreName, const QDBusUnixFileDescriptor &targetDirFd)
    {
        auto loopLock = std::make_shared<QEventLoopLocker>();

        if (coreName.contains('/'_L1)) { // don't allow anything that could look like a path
            sendErrorReply(QDBusError::AccessDenied);
            return {};
        }

        auto tmpDir = std::make_unique<QTemporaryDir>(QDir::tempPath() + "/drkonqi-coredump-excavator"_L1);
        if (!tmpDir->isValid()) {
            qWarning() << "tmpdir not valid";
            sendErrorReply(QDBusError::InternalError, "Failed to create temporary directory"_L1);
            return {};
        }

        const QString coreFileDir = tmpDir->path();
        const QString coreFileTarget = tmpDir->filePath(CORE_NAME);
        const QString coreFile = COREDUMP_PATH + coreName;

        if (!isAuthorized(coreFile, coreFileTarget)) {
            qWarning() << "not authorized";
            sendErrorReply(QDBusError::AccessDenied);
            return {};
        }

        setDelayedReply(true);

        auto connection = this->connection();
        auto msg = message();

        // Keep the core secure by making it only accessible to owner.
        const auto targetDir = QFileInfo(coreFileTarget).path();
        std::filesystem::permissions(targetDir.toStdString(), std::filesystem::perms::owner_all, std::filesystem::perm_options::replace);

        auto excavator = new CoredumpExcavator(this);
        connect(excavator,
                &CoredumpExcavator::excavated,
                this,
                [msg, tmpDir = std::move(tmpDir), loopLock, connection, excavator, coreFileDir, coreFileTarget, targetDirFd](int exitCode) mutable {
                    excavator->deleteLater();

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

                    if (renameat(sourceDirFd, qUtf8Printable(CORE_NAME), targetDirFd.fileDescriptor(), qUtf8Printable(CORE_NAME)) != 0) {
                        int err = errno;
                        QString errString = u"Failed to rename between directory fds: %1"_s.arg(QString::fromUtf8(strerror(err)));
                        connection.send(msg.createErrorReply(QDBusError::InternalError, errString));
                        return;
                    }

                    auto reply = msg.createReply() << (exitCode == 0 ? CORE_NAME : QString());
                    connection.send(reply);
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
