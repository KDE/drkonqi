/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DRKONQI_STATUSNOTIFIER_H
#define DRKONQI_STATUSNOTIFIER_H

#include <QObject>

class QTimer;

class KStatusNotifierItem;

class CrashedApplication;

class StatusNotifier : public QObject
{
    Q_OBJECT

public:
    explicit StatusNotifier(QObject *parent = nullptr);
    ~StatusNotifier() override;

    bool activationAllowed() const;
    void setActivationAllowed(bool allowed);

    void show();
    void notify();

    static bool notificationServiceRegistered();

Q_SIGNALS:
    void expired();
    void activated();

private:
    static bool canBeRestarted(CrashedApplication *app);

    QTimer *m_autoCloseTimer = nullptr;
    KStatusNotifierItem *m_sni = nullptr;

    bool m_activationAllowed = true;

    QString m_title;
};

#endif // DRKONQI_STATUSNOTIFIER_H
