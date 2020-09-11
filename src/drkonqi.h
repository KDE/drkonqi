/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DRKONQI_H
#define DRKONQI_H

#include <QString>

class QWidget;

class SystemInformation;
class DebuggerManager;
class CrashedApplication;
class AbstractDrKonqiBackend;

class DrKonqi
{
public:
    static bool init();

    static SystemInformation *systemInformation();
    static DebuggerManager *debuggerManager();
    static CrashedApplication *crashedApplication();

    static void saveReport(const QString & reportText, QWidget *parent = nullptr);
    static void shutdownSaveReport();
    static void setSignal(int signal);
    static void setAppName(const QString &appName);
    static void setAppPath(const QString &appPath);
    static void setAppVersion(const QString &appVersion);
    static void setBugAddress(const QString &bugAddress);
    static void setProgramName(const QString &programName);
    static void setPid(int pid);
    static void setKdeinit(bool kdeinit);
    static void setSafer(bool safer);
    static void setRestarted(bool restarted);
    static void setKeepRunning(bool keepRunning);
    static void setThread(int thread);

    static int signal();
    static const QString &appName();
    static const QString &appPath();
    static const QString &appVersion();
    static const QString &bugAddress();
    static const QString &programName();
    static int pid();
    static bool isKdeinit();
    static bool isSafer();
    static bool isRestarted();
    static bool isKeepRunning();
    static int thread();
    static bool ignoreQuality();
    static const QString &kdeBugzillaURL();

private:
    DrKonqi();
    ~DrKonqi();
    static DrKonqi *instance();

    SystemInformation *m_systemInformation = nullptr;
    AbstractDrKonqiBackend *m_backend = nullptr;

    int m_signal;
    QString m_appName;
    QString m_appPath;
    QString m_appVersion;
    QString m_bugAddress;
    QString m_programName;
    int m_pid;
    bool m_kdeinit;
    bool m_safer;
    bool m_restarted;
    bool m_keepRunning;
    int m_thread;
};

#endif
