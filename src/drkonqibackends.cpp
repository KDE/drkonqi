/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "drkonqibackends.h"

#include <cstdlib>
#include <cerrno>
#include <sys/types.h>
#include <signal.h>

#include <QTimer>
#include <QDir>
#include <QRegularExpression>

#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <QStandardPaths>
#include "drkonqi_debug.h"

#include "crashedapplication.h"
#include "debugger.h"
#include "debuggermanager.h"
#include "backtracegenerator.h"
#include "drkonqi.h"

#ifdef Q_OS_MACOS
#include <AvailabilityMacros.h>
#endif

AbstractDrKonqiBackend::~AbstractDrKonqiBackend()
{
}

bool AbstractDrKonqiBackend::init()
{
    m_crashedApplication = constructCrashedApplication();
    m_debuggerManager = constructDebuggerManager();
    return true;
}


KCrashBackend::KCrashBackend()
    : QObject(), AbstractDrKonqiBackend(), m_state(ProcessRunning)
{
}

KCrashBackend::~KCrashBackend()
{
    continueAttachedProcess();
}

bool KCrashBackend::init()
{
    AbstractDrKonqiBackend::init();

    //check whether the attached process exists and whether we have permissions to inspect it
    if (crashedApplication()->pid() <= 0) {
        qCWarning(DRKONQI_LOG) << "Invalid pid specified";
        return false;
    }

#if !defined(Q_OS_WIN32)
    if (::kill(crashedApplication()->pid(), 0) < 0) {
        switch (errno) {
        case EPERM:
            qCWarning(DRKONQI_LOG) << "DrKonqi doesn't have permissions to inspect the specified process";
            break;
        case ESRCH:
            qCWarning(DRKONQI_LOG) << "The specified process does not exist.";
            break;
        default:
            break;
        }
        return false;
    }

    //--keeprunning means: generate backtrace instantly and let the process continue execution
    if(DrKonqi::isKeepRunning()) {
        stopAttachedProcess();
        debuggerManager()->backtraceGenerator()->start();
        connect(debuggerManager(), &DebuggerManager::debuggerFinished, this, &KCrashBackend::continueAttachedProcess);
    } else {
        connect(debuggerManager(), &DebuggerManager::debuggerStarting, this, &KCrashBackend::onDebuggerStarting);
        connect(debuggerManager(), &DebuggerManager::debuggerFinished, this, &KCrashBackend::onDebuggerFinished);

        //stop the process to avoid high cpu usage by other threads (bug 175362).
        //if the process was started by kdeinit, we need to wait a bit for KCrash
        //to reach the alarm(0); call in kdeui/util/kcrash.cpp line 406 or else
        //if we stop it before this call, pending alarm signals will kill the
        //process when we try to continue it.
        QTimer::singleShot(2000, this, &KCrashBackend::stopAttachedProcess);
    }
#endif

    //Handle drkonqi crashes
    s_pid = crashedApplication()->pid(); //copy pid for use by the crash handler, so that it is safer
    KCrash::setEmergencySaveFunction(emergencySaveFunction);

    return true;
}

CrashedApplication *KCrashBackend::constructCrashedApplication()
{
    CrashedApplication *a = new CrashedApplication(this);
    a->m_datetime = QDateTime::currentDateTime();
    a->m_name = DrKonqi::programName();
    a->m_version = DrKonqi::appVersion();
    a->m_reportAddress = BugReportAddress(DrKonqi::bugAddress());
    a->m_pid = DrKonqi::pid();
    a->m_signalNumber = DrKonqi::signal();
    a->m_restarted = DrKonqi::isRestarted();
    a->m_thread = DrKonqi::thread();

    //try to determine the executable that crashed
    const QString procPath(QStringLiteral("/proc/%1").arg(a->m_pid));
    const QString exeProcPath(procPath + QStringLiteral("/exe"));
    if (QFileInfo(exeProcPath).exists()) {
        //on linux, the fastest and most reliable way is to get the path from /proc
        qCDebug(DRKONQI_LOG) << "Using /proc to determine executable path";
        const QString exePath = QFile::symLinkTarget(exeProcPath);

        a->m_executable.setFile(exePath);
        if (DrKonqi::isKdeinit() ||
            a->m_executable.fileName().startsWith(QLatin1String("python")) ) {
            a->m_fakeBaseName = DrKonqi::appName();
        }

        QDir mapFilesDir(procPath + QStringLiteral("/map_files"));
        mapFilesDir.setFilter(mapFilesDir.filter() | QDir::System); // proc is system!

        // "/bin/foo (deleted)" is how the kernel tells us that a file has been deleted since
        // it was mmap'd.
        QRegularExpression expression(QStringLiteral("(?<path>.+) \\(deleted\\)$"));
        // For the map_files we filter only .so files to ensure that
        // we don't trip over cache files or the like, as a result we
        // manually need to check if the main exe was deleted and add
        // it.
        // NB: includes .so* and .py* since we also implicitly support snakes to
        //   a degree
        QRegularExpression soExpression(QStringLiteral("(?<path>.+\\.(so|py)([^/]*)) \\(deleted\\)$"));

        bool hasDeletedFiles = false;

        const auto exeMatch = expression.match(exePath);
        if (exeMatch.isValid() && exeMatch.hasMatch()) {
            hasDeletedFiles = true;
        }

        const auto list = mapFilesDir.entryInfoList();
        for (auto it = list.constBegin(); !hasDeletedFiles && it != list.constEnd(); ++it) {
            const auto match = soExpression.match(it->symLinkTarget());
            if (!match.isValid() || !match.hasMatch()) {
                continue;
            }
            const QString path = match.captured(QStringLiteral("path"));
            if (path.startsWith(QStringLiteral("/memfd"))) {
                // Qml.so's JIT shows up under memfd. This is a false positive against our regex.
                continue;
            }
            hasDeletedFiles = true;
        }

        a->m_hasDeletedFiles = hasDeletedFiles;

        qCDebug(DRKONQI_LOG) << "exe" << exePath << "has deleted files:" << hasDeletedFiles;
    } else {
        if ( DrKonqi::isKdeinit() ) {
            a->m_executable = QFileInfo(QStandardPaths::findExecutable(QStringLiteral("kdeinit5")));
            a->m_fakeBaseName = DrKonqi::appName();
        } else {
            QFileInfo execPath(DrKonqi::appName());
            if ( execPath.isAbsolute() ) {
                a->m_executable = execPath;
            } else if ( !DrKonqi::appPath().isEmpty() ) {
                QDir execDir(DrKonqi::appPath());
                a->m_executable = execDir.absoluteFilePath(execPath.fileName());
            } else {
                a->m_executable = QFileInfo(QStandardPaths::findExecutable(execPath.fileName()));
            }
        }
    }

    qCDebug(DRKONQI_LOG) << "Executable is:" << a->m_executable.absoluteFilePath();
    qCDebug(DRKONQI_LOG) << "Executable exists:" << a->m_executable.exists();

    return a;
}

DebuggerManager *KCrashBackend::constructDebuggerManager()
{
    QList<Debugger> internalDebuggers = Debugger::availableInternalDebuggers(QStringLiteral("KCrash"));
    KConfigGroup config(KSharedConfig::openConfig(), "DrKonqi");
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED > 1070
    QString defaultDebuggerName = config.readEntry("Debugger", QStringLiteral("lldb"));
#elif !defined(Q_OS_WIN)
    QString defaultDebuggerName = config.readEntry("Debugger", QStringLiteral("gdb"));
#else
    QString defaultDebuggerName = config.readEntry("Debugger", QStringLiteral("cdb"));
#endif

    Debugger firstKnownGoodDebugger, preferredDebugger;
    foreach (const Debugger & debugger, internalDebuggers) {
        qCDebug(DRKONQI_LOG) << "Check debugger if"
                             << debugger.displayName() << "[" << debugger.codeName() << "]"
                             << "is installed:" << debugger.isInstalled();
        if (!firstKnownGoodDebugger.isValid() && debugger.isInstalled()) {
            firstKnownGoodDebugger = debugger;
        }
        if (debugger.codeName() == defaultDebuggerName) {
            preferredDebugger = debugger;
        }
        if (firstKnownGoodDebugger.isValid() && preferredDebugger.isValid()) {
            break;
        }
    }

    if (!preferredDebugger.isInstalled()) {
        if (firstKnownGoodDebugger.isValid()) {
            preferredDebugger = firstKnownGoodDebugger;
        } else {
            qCWarning(DRKONQI_LOG) << "Unable to find an internal debugger that can work with the KCrash backend";
        }
    }

    qCDebug(DRKONQI_LOG) << "Using debugger:" << preferredDebugger.codeName();
    return new DebuggerManager(preferredDebugger, Debugger::availableExternalDebuggers(QStringLiteral("KCrash")), this);
}

void KCrashBackend::stopAttachedProcess()
{
    if (m_state == ProcessRunning) {
        qCDebug(DRKONQI_LOG) << "Sending SIGSTOP to process";
        ::kill(crashedApplication()->pid(), SIGSTOP);
        m_state = ProcessStopped;
    }
}

void KCrashBackend::continueAttachedProcess()
{
    if (m_state == ProcessStopped) {
        qCDebug(DRKONQI_LOG) << "Sending SIGCONT to process";
        ::kill(crashedApplication()->pid(), SIGCONT);
        m_state = ProcessRunning;
    }
}

void KCrashBackend::onDebuggerStarting()
{
    continueAttachedProcess();
    m_state = DebuggerRunning;
}

void KCrashBackend::onDebuggerFinished()
{
    m_state = ProcessRunning;
    stopAttachedProcess();
}

//static
qint64 KCrashBackend::s_pid = 0;

//static
void KCrashBackend::emergencySaveFunction(int signal)
{
    // In case drkonqi itself crashes, we need to get rid of the process being debugged,
    // so we kill it, no matter what its state was.
    Q_UNUSED(signal);
    ::kill(s_pid, SIGKILL);
}
