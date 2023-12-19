// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "automaticcoredumpexcavator.h"

#include <chrono>
#include <filesystem>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDebug>

#include "coredumpexcavator.h"

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

void AutomaticCoredumpExcavator::excavateFrom(const QString &coredumpFilename)
{
    if (!m_coreDir) {
        m_coreDir = std::make_unique<QTemporaryDir>(QDir::tempPath() + u"/drkonqi-core"_s);
        Q_ASSERT(m_coreDir->isValid());
        if (!m_coreDir->isValid()) {
            Q_EMIT failed();
            return;
        }
        // Keep the core to ourself.
        std::filesystem::permissions(m_coreDir->path().toStdString(), std::filesystem::perms::owner_all, std::filesystem::perm_options::replace);
    }

    const auto coreFileTarget = m_coreDir->filePath(u"core"_s);
    const auto coredumpFileInfo = QFileInfo(coredumpFilename);

    if (coredumpFileInfo.isReadable()) {
        auto excavator = new CoredumpExcavator(this);
        connect(excavator, &CoredumpExcavator::excavated, this, [this, coreFileTarget](int exitCode) {
            if (exitCode != 0) {
                qWarning() << "Failed to excavate core from file:" << exitCode;
                Q_EMIT failed();
                return;
            }
            Q_EMIT excavated(coreFileTarget);
        });
        excavator->excavateFromTo(coredumpFilename, coreFileTarget);
    } else {
        if (QFile::exists(coreFileTarget)) {
            qDebug() << "Core already exists, returning early";
            Q_EMIT excavated(coreFileTarget);
            return;
        }

        auto msg = QDBusMessage::createMethodCall("org.kde.drkonqi"_L1, "/"_L1, "org.kde.drkonqi"_L1, "excavateFrom"_L1);
        msg << coredumpFileInfo.fileName(); // temp dir is constructed and managed by helper, no need to pass our presumed path in
        static const auto connection = QDBusConnection::connectToBus(QDBusConnection::SystemBus, "drkonqi-polkit-system-connection"_L1);
        connection.interface()->setTimeout(std::chrono::milliseconds(5min).count());
        auto watcher = new QDBusPendingCallWatcher(connection.asyncCall(msg));
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, coreFileTarget] {
            watcher->deleteLater();
            QDBusReply<QString> reply = *watcher;
            qWarning() << reply.isValid() << reply.error() << reply.value();
            if (!reply.isValid() || reply.value().isEmpty()) {
                qWarning() << "Failed to excavate core as admin:" << reply.error();
                Q_EMIT failed();
                return;
            }

            const QFileInfo excavatedCoreInfo(reply.value());
            if (!QFile::rename(excavatedCoreInfo.filePath(), coreFileTarget)) {
                qWarning() << "Failed to move excavated core to target location" << excavatedCoreInfo << coreFileTarget;
                Q_EMIT failed();
                return;
            }
            if (!QDir().remove(excavatedCoreInfo.path())) {
                qWarning() << "Failed to remove polkit excavation directory";
            }

            Q_EMIT excavated(coreFileTarget);
        });
    }
}
