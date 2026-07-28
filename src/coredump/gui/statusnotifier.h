/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DRKONQI_STATUSNOTIFIER_H
#define DRKONQI_STATUSNOTIFIER_H

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QPointer>

class QTimer;

class KNotification;
class KStatusNotifierItem;

class CrashedApplication;

struct NotificationInfo {
    QPointer<KNotification> notification;
    QDateTime spawnedTime;
};

class StatusNotifier : public QObject
{
    Q_OBJECT

public:
    enum class Activation {
        NotAllowed,
        Allowed,
        AlreadySubmitting
    };

    explicit StatusNotifier(QObject *parent = nullptr);
    ~StatusNotifier() override;

    void show();
    void notify(Activation activation);

    static bool notificationServiceRegistered();

Q_SIGNALS:
    void expired();
    void activated(pid_t pid);
    void sentryActivated(pid_t pid);
    void trayActivated();

private:
    static bool canBeRestarted(CrashedApplication *app);

    KStatusNotifierItem *m_sni = nullptr;
    QMap<QString, NotificationInfo> m_notifications;
    QString m_title;
};

#endif // DRKONQI_STATUSNOTIFIER_H
