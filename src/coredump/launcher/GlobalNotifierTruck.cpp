// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2025 Harald Sitter <sitter@kde.org>

#include "GlobalNotifierTruck.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KNotification>
#include <KService>

#include "../coredump.h"
#include "decode.h"

using namespace Qt::StringLiterals;

namespace
{
KService::Ptr serviceForUnitName(const QString &unitName)
{
    auto serviceName = unitNameToServiceName(unitName);
    if (serviceName.isEmpty()) {
        return {};
    }

    if (auto service = KService::serviceByMenuId(serviceName.toString() + ".desktop"_L1); service) {
        return service;
    }
    if (unitName.endsWith("@autostart.service"_L1)) {
        if (auto file = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, u"autostart/%1.desktop"_s.arg(serviceName)); !file.isEmpty()) {
            if (auto service = new KService(file); service->isValid()) {
                return KService::Ptr(service);
            }
        }
    }

    return {};
}
} // namespace

bool GlobalNotifierTruck::handle(const Coredump &dump)
{
#if !defined(WITH_GLOBAL_NOTIFIER)
    return false;
#endif

    if (!dump.m_rawData.value(dump.keyPickup()).isEmpty()) {
        // Pickups are currently not supported for notify handling. The problem is that we don't know if we already
        // notified on a crash or not because we have no persistent storage. To fix this we'd probably need to
        // rejigger things substantially and insert a pickup key into the journal that the processor can then look for.
        // Except then the processor needs to hold on to dumps until the entire journal is processed. All a bit awkward.
        return false;
    }

    struct Unit {
        Unit(const QStringView &exe, const QByteArrayView &cursor, const QByteArrayView &systemUnit, const QByteArrayView &userUnit)
            : m_cursor(QString::fromUtf8(cursor))
            , m_name(QString::fromUtf8(userUnit.isEmpty() ? systemUnit : userUnit))
            , m_service(serviceForUnitName(m_name))
            , m_exe(exe)
        {
        }

        [[nodiscard]] bool isApp()
        {
            return m_service != nullptr;
        }

        [[nodiscard]] QString name()
        {
            return m_service ? m_service->name() : m_exe;
        }

        [[nodiscard]] QString iconName()
        {
            return m_service ? m_service->icon() : QString();
        }

        QString m_cursor;

    private:
        QString m_name;
        KService::Ptr m_service;
        QString m_exe;
    };

    // Be mindful of when QEventLoopLocker start and when they end! Specifically lambdas need suitable scoping so they eventually get destroyed.
    qApp->setQuitLockEnabled(true);

    // Mind that m_cursor of the dump is empty because we don't resolve this from the journal but receive it as json from the processor.
    // As such we need to fetch the cursor from the payload. NOT the dump directly.
    Unit unit(dump.exe, dump.m_rawData.value(dump.keyCursor()), dump.m_rawData.value("COREDUMP_UNIT"_ba), dump.m_rawData.value("COREDUMP_USER_UNIT"_ba));

    auto notification = new KNotification(u"applicationCrash"_s);

    if (unit.isApp()) {
        notification->setTitle(i18nc("@title", "Application Crash", unit.name()));
        notification->setIconName(unit.iconName());
    } else {
        notification->setTitle(i18nc("@title service refers to a background service such as kwalletd or kded", "Service Crash", unit.name()));
    }

    notification->setText(xi18nc("@info", "<command>%1</command> has encountered a fatal error and was closed.", unit.name()));
    auto detailsAction = notification->addAction(i18nc("@action:button show crash details", "Details"));
    connect(detailsAction, &KNotificationAction::activated, notification, [this, unit, loopLocker = QEventLoopLocker()]() {
        auto job = new KIO::CommandLauncherJob(u"drkonqi-coredump-gui"_s, {unit.m_cursor}, this);
        connect(job, &KJob::result, job, [loopLocker = QEventLoopLocker()](KJob *job) {
            if (job->error()) {
                auto errorNotification = KNotification::event(KNotification::Error,
                                                              i18nc("@title", "Failed to Launch"),
                                                              i18nc("@info", "Could not launch the Crashed Process Viewer."),
                                                              u"tools-report-bug"_s);
                connect(errorNotification, &KNotification::destroyed, errorNotification, [loopLocker = QEventLoopLocker()]() {
                    // Only here to scope the loopLocker. Nothing to actually do.
                });
                errorNotification->sendEvent();
                qWarning() << "Failed to launch drkonqi-coredump-gui:" << job->errorString();
            }
        });
        job->start();
    });

    notification->sendEvent();

    // KNotification internally depends on an eventloop to communicate over dbus. We therefore start the
    // eventloop here. ::handle() is expected to be blocking so this has no adverse effects.
    // The eventloop is either exited when the notification gets closed or when the debugger has started.
    qApp->exec();
    return true;
}
