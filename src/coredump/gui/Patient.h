// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QFileInfo>
#include <QJsonObject>
#include <QObject>

#include <KOSRelease>

#include <automaticcoredumpexcavator.h>

class Coredump;
class Patient : public QObject
{
    Q_OBJECT

    QString m_origCoreFilename;
    QFileInfo m_coreFileInfo;

#define MEMBER_PROPERTY(type, name)                                                                                                                            \
    Q_PROPERTY(type name MEMBER m_##name NOTIFY changed)                                                                                                       \
    type m_##name

    // NB: these all share the same changed signal but its only ever emitted once.
    // They are effectively CONSTANT, but not really because they change during construction.
    MEMBER_PROPERTY(int, signal) = -1;
    MEMBER_PROPERTY(QString, appName);
    MEMBER_PROPERTY(pid_t, pid) = 0;
    MEMBER_PROPERTY(time_t, timestamp) = 0;
#undef MEMBER_PROPERTY
    Q_PROPERTY(bool canDebug READ canDebug NOTIFY changed)
    Q_PROPERTY(bool canReport READ canReport NOTIFY changed)
    Q_PROPERTY(QString dateTime READ dateTime NOTIFY changed)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QString faultEntityName READ faultEntityName CONSTANT)
    Q_PROPERTY(QString journalCursor MEMBER m_journalCursor CONSTANT)
public:
    explicit Patient(const Coredump &dump);

    QStringList coredumpctlArguments(const QString &command) const;

    bool canDebug() const;
    Q_INVOKABLE [[nodiscard]] QString reasonForNoDebug() const;
    Q_INVOKABLE void debug();
    QString dateTime() const;
    QString iconName() const;
    [[nodiscard]] QString faultEntityName() const;
    [[nodiscard]] bool canReport();
    Q_INVOKABLE [[nodiscard]] QString reasonForNoReport() const;
    Q_INVOKABLE void report();

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void launchDebugger();

private:
    const QByteArray m_coredumpExe;
    const QByteArray m_coredumpCom;
    QString m_iconName;
    std::unique_ptr<AutomaticCoredumpExcavator> m_excavator;
    QByteArray m_systemUnit;
    QByteArray m_userUnit;
    KOSRelease m_osRelease;
    struct FaultContext {
        enum class Entity {
            Flatpak,
            Snap,
            KDE,
            Distro // always when unknown
        };
        Entity entity;
        QString name;
        QString drkonqiMetadataPath = {}; // NOLINT this is not a redundant init!
        QJsonObject drkonqiMetadata = {}; // NOLINT this is not a redundant init!
        bool reportedToKDE = false;
    } m_faultContext;
    QString m_journalCursor;
};

Q_DECLARE_METATYPE(time_t)
Q_DECLARE_METATYPE(pid_t)
