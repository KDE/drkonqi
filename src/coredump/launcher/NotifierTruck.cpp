/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
*/

#include "NotifierTruck.h"

#include <KNotification>
#include <QFile>
#include <QProcess>

#include <sys/types.h>
#include <unistd.h>

#include "../coredump.h"

bool NotifyTruck::handle(const Coredump &dump)
{
    if (!QFile::exists(dump.filename)) {
        auto notification = new KNotification(QStringLiteral("applicationcrash"));
        notification->setTitle(QStringLiteral("The warpcore has gone missing"));
        notification->setText(QStringLiteral("%1 [%2] crashed but has no core file").arg(dump.exe, QString::number(dump.pid)));
        notification->setFlags(KNotification::DefaultEvent | KNotification::SkipGrouping);
        notification->sendEvent();
        return true;
    }

    auto notification = new KNotification(QStringLiteral("applicationcrash"));
    notification->setTitle(QStringLiteral("He's dead, Jim"));
    notification->setText(QStringLiteral("%1 [%2]").arg(dump.exe, QString::number(dump.pid)));
    notification->setActions({QStringLiteral("gdb")});

    const auto pid = dump.pid;
    const auto uid = dump.uid;
    QObject::connect(notification, static_cast<void (KNotification::*)(unsigned int)>(&KNotification::activated), notification, [pid, uid]() {
        QStringList args{QStringLiteral("--nofork"), QStringLiteral("-e"), QStringLiteral("coredumpctl"), QStringLiteral("gdb"), QString::number(pid)};
        // if the crash isn't from the current user then route it through pkexec
        if (uid != getuid()) {
            args = QStringList{QStringLiteral("--nofork"), QStringLiteral("-e"), QStringLiteral("pkexec coredumpctl gdb %1").arg(pid)};
        }
        QProcess::startDetached(QStringLiteral("konsole"), args);
    });

    notification->setFlags(KNotification::DefaultEvent | KNotification::SkipGrouping);
    notification->sendEvent();

    return true;
}
