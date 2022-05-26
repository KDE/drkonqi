// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>

class Coredump;
class Patient : public QObject
{
    Q_OBJECT

#define MEMBER_PROPERTY(type, name)                                                                                                                            \
    Q_PROPERTY(type name MEMBER m_##name NOTIFY changed)                                                                                                       \
    type m_##name

    // NB: these all share the same changed signal but its only ever emitted once.
    // They are effectively CONSTANT, but not really because they change during construction.
    MEMBER_PROPERTY(int, signal) = -1;
    MEMBER_PROPERTY(QString, appName);
    MEMBER_PROPERTY(pid_t, pid) = 0;
    MEMBER_PROPERTY(bool, canDebug) = false;
    MEMBER_PROPERTY(time_t, timestamp) = 0;
#undef MEMBER_PROPERTY
    Q_PROPERTY(QString dateTime READ dateTime NOTIFY changed)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
public:
    explicit Patient(const Coredump &dump);

    QStringList coredumpctlArguments(const QString &command) const;

    Q_INVOKABLE void debug() const;
    QString dateTime() const;
    QString iconName() const;

Q_SIGNALS:
    void changed();

private:
    const QByteArray m_coredumpExe;
    const QByteArray m_coredumpCom;
    QString m_iconName;
};

Q_DECLARE_METATYPE(time_t)
Q_DECLARE_METATYPE(pid_t)
