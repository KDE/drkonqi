/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "statusnotifier.h"

#include <QAction>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QMenu>

#include <KLocalizedString>
#include <KNotification>
#include <KStatusNotifierItem>

#include "crashedapplication.h"
#include "drkonqi.h"
#include "statusnotifier_activationclosetimer.h"

StatusNotifier::StatusNotifier(QObject *parent)
    : QObject(parent)
    , m_sni(new KStatusNotifierItem(this))
{
    CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    // this is used for both the SNI tooltip as well as the notification
    m_title = i18nc("Placeholder is an application name; it crashed", "%1 Closed Unexpectedly", crashedApp->name());
}

StatusNotifier::~StatusNotifier() = default;

bool StatusNotifier::activationAllowed() const
{
    return m_activationAllowed;
}

void StatusNotifier::setActivationAllowed(bool allowed)
{
    m_activationAllowed = allowed;
}

void StatusNotifier::show()
{
    CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    connect(this, &StatusNotifier::activated, this, &StatusNotifier::deleteLater);
    // The expiring the notifier doesn't necessarily quit immediately, make sure to hide the SNI by deleting it (there
    // is no way to hide an SNI).
    connect(this, &StatusNotifier::expired, m_sni, &QObject::deleteLater);

    m_sni->setTitle(m_title);
    m_sni->setIconByName(QStringLiteral("tools-report-bug"));
    m_sni->setStatus(KStatusNotifierItem::Active);
    m_sni->setCategory(KStatusNotifierItem::SystemServices);
    m_sni->setToolTip(m_sni->iconName(), m_sni->title(), i18n("Please report this error to help improve this software."));
    connect(m_sni, &KStatusNotifierItem::activateRequested, this, &StatusNotifier::activated);

    // you cannot turn off that "Do you really want to quit?" message, so we'll add our own below
    m_sni->setStandardActionsEnabled(false);

    auto *sniMenu = new QMenu();
    auto *action = new QAction(QIcon::fromTheme(QStringLiteral("tools-report-bug")), i18n("Report &Bug"), nullptr);
    connect(action, &QAction::triggered, this, &StatusNotifier::activated);
    sniMenu->addAction(action);
    sniMenu->setDefaultAction(action);

    if (canBeRestarted(crashedApp)) {
        action = new QAction(QIcon::fromTheme(QStringLiteral("system-reboot")), i18n("&Restart Application"), nullptr);
        connect(action, &QAction::triggered, crashedApp, &CrashedApplication::restart);
        // once restarted successfully, disable restart option
        connect(crashedApp, &CrashedApplication::restarted, action, [action](bool success) {
            action->setEnabled(!success);
        });
        sniMenu->addAction(action);
    }

    sniMenu->addSeparator();

    action = new QAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18nc("Allows the user to hide this notifier item", "Hide"), nullptr);
    connect(action, &QAction::triggered, this, &StatusNotifier::expired);
    sniMenu->addAction(action);

    m_sni->setContextMenu(sniMenu);

    // Depending on the environmental state we may auto-activate or auto-close the SNI.
    auto timer = new ActivationCloseTimer(this);
    connect(timer, &ActivationCloseTimer::autoActivate, this, &StatusNotifier::activated);
    connect(timer, &ActivationCloseTimer::autoClose, this, &StatusNotifier::expired);
    auto dbusWatcher = new DBusServiceWatcher(timer);
    auto idleWatcher = new IdleWatcher(timer);
    timer->start(dbusWatcher, idleWatcher);
}

void StatusNotifier::notify()
{
    CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    const QString title = m_activationAllowed ? m_title : crashedApp->name();
    const QString message = m_activationAllowed ? i18nc("Notification text", "Please report this error to help improve this software.")
                                                : i18nc("Notification text", "The application closed unexpectedly.");

    KNotification *notification = KNotification::event(QStringLiteral("applicationcrash"),
                                                       title,
                                                       message,
                                                       QStringLiteral("tools-report-bug"),
                                                       nullptr,
                                                       KNotification::DefaultEvent | KNotification::SkipGrouping);

    QStringList actions;
    if (m_activationAllowed) {
        actions << i18nc("Notification action button, keep short", "Report Bug");
    }
    if (canBeRestarted(crashedApp)) {
        actions << i18nc("Notification action button, keep short", "Restart App");
    }

    notification->setActions(actions);

    connect(notification, static_cast<void (KNotification::*)(unsigned int)>(&KNotification::activated), this, [this, crashedApp](int actionIndex) {
        if (actionIndex == 1 && m_activationAllowed) {
            Q_EMIT activated();
        } else if (canBeRestarted(crashedApp)) {
            crashedApp->restart();
        }
    });

    // when the SNI disappears you won't be able to interact with the notification anymore anyway, so close it
    if (m_activationAllowed) {
        connect(this, &StatusNotifier::activated, notification, &KNotification::close);
        connect(this, &StatusNotifier::expired, notification, &KNotification::close);
    } else {
        // No SNI means we should quit when the notification is gone
        connect(notification, &KNotification::closed, this, &StatusNotifier::expired);
    }
}

bool StatusNotifier::notificationServiceRegistered()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.Notifications"));
}

bool StatusNotifier::canBeRestarted(CrashedApplication *app)
{
    return !app->hasBeenRestarted() && app->fakeExecutableBaseName() != QLatin1String("drkonqi");
}
