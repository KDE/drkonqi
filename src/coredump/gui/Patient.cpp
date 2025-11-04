// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

#include "Patient.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>

#include <KApplicationTrader>
#include <KLocalizedString>
#include <KOSRelease>
#include <KShell>
#include <KTerminalLauncherJob>

#include <drkonqipaths.h>
#include <metadata.h>

#include "../coredump/coredump.h"

using namespace Qt::StringLiterals;

Patient::Patient(const Coredump &dump)
    : m_origCoreFilename(QString::fromUtf8(dump.m_rawData.value("COREDUMP_FILENAME")))
    , m_coreFileInfo(m_origCoreFilename)
    , m_signal(dump.m_rawData["COREDUMP_SIGNAL"].toInt())
    , m_appName(QFileInfo(dump.exe).fileName())
    , m_pid(dump.pid)
    , m_timestamp(dump.m_rawData["COREDUMP_TIMESTAMP"].toLong())
    , m_coredumpExe(dump.m_rawData["COREDUMP_EXE"])
    , m_coredumpCom(dump.m_rawData["COREDUMP_COMM"])
    , m_faultContext([this, &dump]() -> FaultContext {
        const QString userUnit = QString::fromUtf8(dump.m_rawData.value("COREDUMP_USER_UNIT"_ba));
        const QString systemUnit = QString::fromUtf8(dump.m_rawData.value("COREDUMP_UNIT"_ba));

        for (const auto &unit : {userUnit, systemUnit}) {
            constexpr auto flatpakPrefix = "app-flatpak-"_L1;
            if (!unit.startsWith(flatpakPrefix)) {
                continue;
            }

            const auto flatpakName = [&unit, &flatpakPrefix] {
                const auto end = unit.mid(flatpakPrefix.size());
                return end.left(end.indexOf('-'_L1));
            }();
            return {.entity = FaultContext::Entity::Flatpak, .name = flatpakName};
        }

        for (const auto &unit : {userUnit, systemUnit}) {
            constexpr auto snapPrefix = "snap."_L1;
            if (!unit.startsWith(snapPrefix)) {
                continue;
            }

            const auto snapName = [&unit, &snapPrefix] {
                const auto end = unit.mid(snapPrefix.size());
                return end.left(end.indexOf('.'_L1));
            }();
            return {.entity = FaultContext::Entity::Snap, .name = snapName};
        }

        const auto drkonqiMetadataPath = Metadata::drkonqiMetadataPath(dump.exe, dump.bootId, dump.timestamp, dump.pid);
        const auto metadata = Metadata::readFromDisk(drkonqiMetadataPath);
        if (!metadata.isEmpty() && !metadata.value(u"kcrash"_s).toObject().isEmpty()) {
            return {
                .entity = FaultContext::Entity::KDE,
                .name = {}, // unused
                .drkonqiMetadataPath = drkonqiMetadataPath,
                .drkonqiMetadata = metadata,
                .reportedToKDE = !metadata.value(Metadata::DRKONQI_KEY).toObject().value(Metadata::SENTRY_EVENT_ID_KEY).toString().isEmpty(),
            };
        }

        return {.entity = FaultContext::Entity::Distro, .name = m_osRelease.prettyName()};
    }())
    , m_journalCursor(QString::fromUtf8(dump.m_cursor))
{
}

QStringList Patient::coredumpctlArguments(const QString &command) const
{
    return {command, u"COREDUMP_FILENAME=%1"_s.arg(m_origCoreFilename)};
}

void Patient::launchDebugger()
{
    const QString arguments = KShell::joinArgs({u"gdb"_s, u"--core=%1"_s.arg(m_coreFileInfo.filePath()), QString::fromUtf8(m_coredumpExe)});
    auto job = new KTerminalLauncherJob(arguments);
    connect(job, &KJob::result, this, [job] {
        job->deleteLater();
        if (job->error() != KJob::NoError) {
            qWarning() << job->errorText();
        }
    });
    job->start();
}

void Patient::debug()
{
    if (!m_excavator) {
        m_excavator = std::make_unique<AutomaticCoredumpExcavator>();
        connect(m_excavator.get(), &AutomaticCoredumpExcavator::failed, this, [this] {
            QMessageBox::critical(qApp ? qApp->activeWindow() : nullptr,
                                  i18nc("@title", "Failure"),
                                  i18nc("@info", "Failed to access crash data for unknown reasons."));
            m_excavator.release()->deleteLater();
        });
        connect(m_excavator.get(), &AutomaticCoredumpExcavator::excavated, this, [this](const QString &corePath) {
            m_coreFileInfo = QFileInfo(corePath);
            launchDebugger();
        });
        m_excavator->excavateFrom(m_coreFileInfo.filePath());
        return;
    }
    if (m_coreFileInfo.isReadable()) {
        launchDebugger();
    }
    // else supposedly still waiting for excavator
}

QString Patient::dateTime() const
{
    QDateTime time;
    time.setMSecsSinceEpoch(m_timestamp / 1000);
    return QLocale().toString(time, QLocale::LongFormat);
}

QString Patient::iconName() const
{
    // Caching it because it's an N² look-up and there generally are tons of duplicates
    static QHash<QString, QString> s_cache;
    const QString executable = m_appName;
    auto it = s_cache.find(executable);
    if (it == s_cache.end()) {
        const auto servicesFound = KApplicationTrader::query([&executable](const KService::Ptr &service) {
            return QFileInfo(service->exec()).fileName() == executable;
        });

        QString iconName;
        if (servicesFound.isEmpty()) {
            iconName = QStringLiteral("applications-science");
        } else {
            iconName = servicesFound.constFirst()->icon();
        }
        it = s_cache.insert(executable, iconName);
    }
    return *it;
}

bool Patient::canDebug() const
{
    switch (m_faultContext.entity) {
    case FaultContext::Entity::Flatpak:
    case FaultContext::Entity::Snap:
        return false;
    case FaultContext::Entity::Distro:
    case FaultContext::Entity::KDE:
        break; // only applicable for non-sandboxed software from us
    }
    return m_coreFileInfo.exists();
}

[[nodiscard]] QString Patient::reasonForNoDebug() const
{
    switch (m_faultContext.entity) {
    case FaultContext::Entity::Flatpak:
        return i18nc("@info", "Debugging is not possible for crashes of software run inside a Flatpak sandbox at this time.");
    case FaultContext::Entity::Snap:
        return i18nc("@info", "Debugging is not possible for crashes of software run inside a Snap sandbox at this time.");
    case FaultContext::Entity::Distro:
    case FaultContext::Entity::KDE:
        if (!m_coreFileInfo.exists()) {
            return i18nc("@info", "Debugging is no longer possible for this crash. The core dump file is missing.");
        }
        return {};
    }
    return i18nc("@info", "Debugging is no longer possible for this crash.");
}

bool Patient::canReport()
{
    switch (m_faultContext.entity) {
    case FaultContext::Entity::Flatpak:
        return false;
    case FaultContext::Entity::Snap:
        return true;
    case FaultContext::Entity::Distro:
        return !m_osRelease.bugReportUrl().isEmpty();
    case FaultContext::Entity::KDE:
        // We only accept symbolicated reports so canDebug is a pre-condition here.
        return canDebug() && !m_faultContext.reportedToKDE;
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, "Unhandled enum value");
    return false;
}

QString Patient::reasonForNoReport() const
{
    switch (m_faultContext.entity) {
    case FaultContext::Entity::Flatpak:
        return i18nc("@info", "Reporting is not supported for Flatpak applications at this time.");
    case FaultContext::Entity::KDE:
        if (!canDebug()) {
            return reasonForNoDebug();
        }
        return i18nc("@info", "Already reported.");
    case FaultContext::Entity::Snap:
        return {}; // enabled -> simply direct to snap store
    case FaultContext::Entity::Distro:
        if (m_osRelease.bugReportUrl().isEmpty()) {
            return i18nc("@info", "Your distribution has not provided a bug report URL. Please report this to your distribution.");
        }
        return {}; // enabled
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, "Unhandled enum value");
    return QString();
}

void Patient::report()
{
    switch (m_faultContext.entity) {
    case FaultContext::Entity::Flatpak:
        // TODO lookup flatpak origin
        return;
    case FaultContext::Entity::Snap:
        QDesktopServices::openUrl(QUrl(u"https://snapcraft.io/"_s.append(m_faultContext.name)));
        return;
    case FaultContext::Entity::KDE: {
        auto job = new KIO::CommandLauncherJob(
            Paths::drkonqiExe(),
            QStringList{u"--dialog"_s} + Metadata::metadataArguments(m_faultContext.drkonqiMetadata[Metadata::KCRASH_KEY].toObject().toVariantHash()),
            this);
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(u"DRKONQI_BACKEND"_s, u"COREDUMPD"_s);
        env.insert(u"DRKONQI_METADATA_FILE"_s, m_faultContext.drkonqiMetadataPath);
        job->setProcessEnvironment(env);
        job->start();
        // TODO: this is a bit awkward because it allows the user to open the same report multiple times. There is no
        // good way to prevent this though I think.
        return;
    }
    case FaultContext::Entity::Distro:
        QDesktopServices::openUrl(QUrl(m_osRelease.bugReportUrl()));
        return;
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, "Unhandled enum value");
}

[[nodiscard]] QString Patient::faultEntityName() const
{
    switch (m_faultContext.entity) {
    case FaultContext::Entity::Flatpak:
        return i18nc("@info the name of where to report bugs", "Flatpak");
    case FaultContext::Entity::Snap:
        return i18nc("@info the name of where to report bugs", "Snap Store");
    case FaultContext::Entity::KDE:
        return i18nc("@info the name of where to report bugs", "KDE");
    case FaultContext::Entity::Distro:
        return m_osRelease.prettyName();
    }
    Q_ASSERT_X(false, Q_FUNC_INFO, "Unhandled enum value");
    return {};
}

#include "moc_Patient.cpp"
