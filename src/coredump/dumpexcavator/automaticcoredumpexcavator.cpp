// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023-2026 Harald Sitter <sitter@kde.org>

#include "automaticcoredumpexcavator.h"

#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QStandardPaths>

#include <KLocalizedString>

#include "coredumpexcavator.h"

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

void AutomaticCoredumpExcavator::excavateFrom(const QString &coredumpFilename)
{
    if (!m_coreDir || !m_coreDir->isValid()) { // lazy init m_coreDir
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/cores/"_s;
        QDir().mkpath(cacheDir);

        m_coreDir = std::make_unique<QTemporaryDir>(u"%1/%2-XXXXXX"_s.arg(cacheDir, QDateTime::currentDateTime().toString(Qt::ISODateWithMs)));
        Q_ASSERT(m_coreDir->isValid());
        if (!m_coreDir->isValid()) {
            Q_EMIT failed(
                i18nc("diagnostic error. %1 is the specific error message from the system", "Failed to create core directory: %1", m_coreDir->errorString()));
            return;
        }
        // Keep the core to ourself.
        std::filesystem::permissions(m_coreDir->path().toStdString(), std::filesystem::perms::owner_all, std::filesystem::perm_options::replace);
    }

    const auto coreFileTarget = m_coreDir->filePath(u"core"_s);
    const auto coredumpFileInfo = QFileInfo(coredumpFilename);

    auto coreFile = std::make_shared<QFile>(coreFileTarget);
    if (coreFile->exists()) {
        qDebug() << "Core already exists, returning early";
        Q_EMIT excavated(coreFileTarget);
        return;
    }
    if (!coreFile->open(QFile::WriteOnly, QFile::ReadUser | QFile::WriteUser)) {
        qWarning() << "Failed to open coreFileTarget" << coreFileTarget << coreFile->errorString();
        Q_EMIT failed(i18nc("diagnostic error. %1 is the specific error message from the system", "Failed to open core file: %1", coreFile->errorString()));
        return;
    }

    if (!coredumpFileInfo.exists()) {
        qWarning() << "Coredump file does not exist" << coredumpFilename;
        Q_EMIT failed(i18nc("diagnostic error. %1 is the specific error message from the system", "Coredump file does not exist: %1", coredumpFilename));
        return;
    }

    if (coredumpFileInfo.isReadable()) {
        auto excavator = new CoredumpExcavator(this);
        connect(excavator, &CoredumpExcavator::excavated, this, [this, coreFileTarget](int exitCode) {
            if (exitCode != 0) {
                qWarning() << "Failed to excavate core from file:" << exitCode;
                Q_EMIT failed(
                    i18nc("diagnostic error. %1 is the numeric exit code", "Core file extraction process failed with code: %1", QString::number(exitCode)));
                return;
            }
            Q_EMIT excavated(coreFileTarget);
        });
        // Only has one signal!
        excavator->excavateFromTo(coredumpFilename, coreFile);
    } else {
        auto msg = QDBusMessage::createMethodCall("org.kde.drkonqi"_L1, "/"_L1, "org.kde.drkonqi"_L1, "saveCoreToFile"_L1);

        msg << coredumpFileInfo.fileName() << QVariant::fromValue(QDBusUnixFileDescriptor(coreFile->handle()));
        constexpr auto timeout = std::chrono::milliseconds(5min).count();
        auto watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg, timeout));
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, coreFileTarget] {
            watcher->deleteLater();
            QDBusReply<void> reply = *watcher;
            qWarning() << reply.isValid() << reply.error();
            if (!reply.isValid()) {
                m_coreDir = nullptr;
                qWarning() << "Failed to excavate core as admin:" << reply.error();
                Q_EMIT failed(i18nc("diagnostic error. %1 is a dbus error from a polkit helper",
                                    "Elevated core file extraction process failed: %1",
                                    reply.error().message()));
                return;
            }
            Q_EMIT excavated(coreFileTarget);
        });
    }
}

#include "moc_automaticcoredumpexcavator.cpp"
