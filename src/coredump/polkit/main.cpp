// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include <filesystem>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusMetaType>
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
    static constexpr auto ACTION_NAME = "org.kde.drkonqi.excavateFrom"_L1;

public Q_SLOTS:
    QString excavateFrom(const QString &coreName)
    {
        auto loopLock = std::make_shared<QEventLoopLocker>();
        QTemporaryDir tmpDir(QDir::tempPath() + "/drkonqi-coredump-execavator"_L1);
        if (!tmpDir.isValid()) {
            qWarning() << "tmpdir not valid";
            sendErrorReply(QDBusError::InternalError, "Failed to create temporary directory"_L1);
            return {};
        }

        const QString coreFileDir = tmpDir.path();
        const QString coreFileTarget = tmpDir.filePath("core"_L1);
        const QString coreFile = COREDUMP_PATH + coreName;

        if (!isAuthorized(coreFile, coreFileTarget)) {
            qWarning() << "not authorized";
            sendErrorReply(QDBusError::AccessDenied);
            return {};
        }
        tmpDir.setAutoRemove(false);

        setDelayedReply(true);

        auto connection = this->connection();
        auto reply = message().createReply();
        const uint uid = connection.interface()->serviceUid(message().service());

        // Keep the core secure by making it only accessible to owner. This is initially root
        // and will be transferred to the UID of the caller upon excavation.
        const auto targetDir = QFileInfo(coreFileTarget).path();
        std::filesystem::permissions(targetDir.toStdString(), std::filesystem::perms::owner_all, std::filesystem::perm_options::replace);

        auto excavator = new CoredumpExcavator(this);
        connect(excavator,
                &CoredumpExcavator::excavated,
                this,
                [loopLock, connection, reply, excavator, coreFileDir, coreFileTarget, uid](int exitCode) mutable {
                    excavator->deleteLater();

                    // Transfer ownership to caller.
                    if (chown(qUtf8Printable(coreFileDir), uid, -1) != 0) {
                        auto err = errno;
                        qWarning() << "Failed to chown" << coreFileDir << strerror(err);
                    }
                    if (chown(qUtf8Printable(coreFileTarget), uid, -1) != 0) {
                        auto err = errno;
                        qWarning() << "Failed to chown" << coreFileTarget << strerror(err);
                    }

                    reply << (exitCode == 0 ? coreFileTarget : QString());
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
