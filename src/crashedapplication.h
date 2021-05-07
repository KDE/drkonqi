/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef CRASHEDAPPLICATION_H
#define CRASHEDAPPLICATION_H

#include <QDateTime>
#include <QFileInfo>
#include <QObject>

#include "bugreportaddress.h"

class KCrashBackend;

class CrashedApplication : public QObject
{
    Q_OBJECT
public:
    CrashedApplication(int pid,
                       int thread,
                       int signalNumber,
                       const QFileInfo &executable,
                       const QString &version,
                       const BugReportAddress &reportAddress,
                       const QString &name = QString(),
                       const QString &productName = QString(),
                       const QDateTime &datetime = QDateTime::currentDateTime(),
                       bool restarted = false,
                       bool hasDeletedFiles = false,
                       const QString &fakeBaseName = QString(),
                       QObject *parent = nullptr);

    ~CrashedApplication() override;

    /** Returns the crashed program's name, possibly translated (ex. "The KDE Crash Handler") */
    QString name() const;

    /** Returns a QFileInfo with information about the executable that crashed */
    QFileInfo executable() const;

    /** When an application is run via kdeinit, the executable() method returns kdeinit4, but
     * we still need a way to know which is the application that was loaded by kdeinit. So,
     * this method returns the base name of the executable that would have been launched if
     * the app had not been loaded by kdeinit (ex. "plasma-desktop"). If the application was
     * not launched via kdeinit, this method returns executable().baseName();
     */
    QString fakeExecutableBaseName() const;

    /** Returns the version of the crashed program */
    QString version() const;

    /** Returns the address where the bug report for this application should go */
    BugReportAddress bugReportAddress() const;

    /** Bugzialla product name, if the crashed application has explicitly specified that. */
    QString productName() const;

    /** Returns the pid of the crashed program */
    int pid() const;

    /** Returns the signal number that the crashed program received */
    int signalNumber() const;

    /** Returns the name of the signal (ex. SIGSEGV) */
    QString signalName() const;

    bool hasBeenRestarted() const;

    int thread() const;

    const QDateTime &datetime() const;

    /** @returns whether mmap'd files have been deleted, e.g. updated since start of app */
    bool hasDeletedFiles() const;

public Q_SLOTS:
    void restart();

Q_SIGNALS:
    void restarted(bool success);

protected:
    int m_pid;
    int m_signalNumber;
    QString m_name;
    QFileInfo m_executable;
    QString m_fakeBaseName;
    QString m_version;
    BugReportAddress m_reportAddress;
    QString m_productName;
    bool m_restarted;
    int m_thread;
    QDateTime m_datetime;
    bool m_hasDeletedFiles;
};

QString getSuggestedKCrashFilename(const CrashedApplication *app);

#endif // CRASHEDAPPLICATION_H
