/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Parts of this code were originally under the following license:

    * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
    *
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions
    * are met:
    *
    * 1. Redistributions of source code must retain the above copyright
    *    notice, this list of conditions and the following disclaimer.
    * 2. Redistributions in binary form must reproduce the above copyright
    *    notice, this list of conditions and the following disclaimer in the
    *    documentation and/or other materials provided with the distribution.
    *
    * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "drkonqi.h"
#include "drkonqi_debug.h"

#include <QPointer>
#include <QTextStream>
#include <QTimerEvent>
#include <QTemporaryFile>
#include <QFileDialog>

#include <KMessageBox>
#include <KCrash>
#include <KLocalizedString>
#include <KJobWidgets>
#include <kio/filecopyjob.h>
#include <QApplication>

#include "systeminformation.h"
#include "crashedapplication.h"
#include "drkonqibackends.h"
#include "debuggermanager.h"
#include "backtracegenerator.h"

DrKonqi::DrKonqi()
    : m_signal(0)
    , m_pid(0)
    , m_kdeinit(false)
    , m_safer(false)
    , m_restarted(false)
    , m_keepRunning(false)
    , m_thread(0)
{
    m_backend = new KCrashBackend();
    m_systemInformation = new SystemInformation();
}

DrKonqi::~DrKonqi()
{
    delete m_systemInformation;
    delete m_backend;
}

//static
DrKonqi *DrKonqi::instance()
{
    static DrKonqi drKonqiInstance;
    return &drKonqiInstance;
}

//based on KCrashDelaySetHandler from kdeui/util/kcrash.cpp
class EnableCrashCatchingDelayed : public QObject
{
public:
    EnableCrashCatchingDelayed() {
        startTimer(10000); // 10 s
    }
protected:
    void timerEvent(QTimerEvent *event) override {
        qCDebug(DRKONQI_LOG) << "Enabling drkonqi crash catching";
        KCrash::setDrKonqiEnabled(true);
        killTimer(event->timerId());
        this->deleteLater();
    }
};

bool DrKonqi::init()
{
    if (!instance()->m_backend->init()) {
        return false;
    } else { //all ok, continue initialization
        // Set drkonqi to handle its own crashes, but only if the crashed app
        // is not drkonqi already. If it is drkonqi, delay enabling crash catching
        // to prevent recursive crashes (in case it crashes at startup)
        if (crashedApplication()->fakeExecutableBaseName() != QLatin1String("drkonqi")) {
            qCDebug(DRKONQI_LOG) << "Enabling drkonqi crash catching";
            KCrash::setDrKonqiEnabled(true);
        } else {
            new EnableCrashCatchingDelayed;
        }
        return true;
    }
}

//static
SystemInformation *DrKonqi::systemInformation()
{
    return instance()->m_systemInformation;
}

//static
DebuggerManager* DrKonqi::debuggerManager()
{
    return instance()->m_backend->debuggerManager();
}

//static
CrashedApplication *DrKonqi::crashedApplication()
{
    return instance()->m_backend->crashedApplication();
}

//static
void DrKonqi::saveReport(const QString & reportText, QWidget *parent)
{
    if (isSafer()) {
        QTemporaryFile tf;
        tf.setFileTemplate(QStringLiteral("XXXXXX.kcrash.txt"));
        tf.setAutoRemove(false);

        if (tf.open()) {
            QTextStream textStream(&tf);
            textStream << reportText;
            textStream.flush();
            KMessageBox::information(parent, xi18nc("@info",
                                                    "Report saved to <filename>%1</filename>.",
                                                    tf.fileName()));
        } else {
            KMessageBox::sorry(parent, i18nc("@info","Could not create a file in which to save the report."));
        }
    } else {
        QString defname = getSuggestedKCrashFilename(crashedApplication());

        QPointer<QFileDialog> dlg(new QFileDialog(parent, defname));
        dlg->selectFile(defname);
        dlg->setWindowTitle(i18nc("@title:window","Select Filename"));
        dlg->setAcceptMode(QFileDialog::AcceptSave);
        dlg->setFileMode(QFileDialog::AnyFile);
        dlg->setOption(QFileDialog::DontResolveSymlinks, false);
        if (dlg->exec() != QDialog::Accepted) {
            return;
        }

        if (!dlg) {
            //Dialog is invalid, it was probably deleted (ex. via DBus call)
            //return and do not crash
            return;
        }

        QUrl fileUrl;
        if(!dlg->selectedUrls().isEmpty())
            fileUrl = dlg->selectedUrls().first();
        delete dlg;

        if (fileUrl.isValid()) {
            QTemporaryFile tf;
            if (tf.open()) {
                QTextStream ts(&tf);
                ts << reportText;
                ts.flush();
            } else {
                KMessageBox::sorry(parent, xi18nc("@info","Cannot open file <filename>%1</filename> "
                                                          "for writing.", tf.fileName()));
                return;
            }

            // QFileDialog was run with confirmOverwrite, so we can safely
            // overwrite as necessary.
            KIO::FileCopyJob* job = KIO::file_copy(QUrl::fromLocalFile(tf.fileName()),
                                                   fileUrl,
                                                   -1,
                                                   KIO::DefaultFlags | KIO::Overwrite);
            KJobWidgets::setWindow(job, parent);
            if (!job->exec()) {
                KMessageBox::sorry(parent, job->errorString());
            }
        }
    }
}

// Helper functions for the shutdownSaveReport
class ShutdownHelper : public QObject {
    Q_OBJECT
public:
    QString shutdownSaveString;

    void removeOldFilesIn(QDir &dir)
    {
        auto fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot,
                                          QDir::SortFlag::Time | QDir::Reversed);
        for(int i = fileList.size(); i >= 10; i--) {
            auto currentFile = fileList.takeFirst();
            dir.remove(currentFile.fileName());
        }
    }

    void saveReportAndQuit()
    {
        const QString dirname = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        // Try to create the directory to save the logs, if we can't open the directory,
        // just bail out. no need to hold the shutdown process.
        QDir dir(dirname);
        if (!dir.mkpath(dirname)) {
            qApp->quit();
        }

        removeOldFilesIn(dir);
        const QString defname = dirname
                        + QLatin1Char('/') 
                        + QStringLiteral("pid-")
                        + QString::number(DrKonqi::pid())
                        + QLatin1Char('-')
                        + getSuggestedKCrashFilename(DrKonqi::crashedApplication());

        QFile shutdownSaveFile(defname);
        if (shutdownSaveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&shutdownSaveFile);
            ts << shutdownSaveString;
            ts.flush();
            shutdownSaveFile.close();
        }
        deleteLater();
        qApp->quit();
    }

    void appendNewLine(const QString& newLine)
    {
        shutdownSaveString += newLine;
    }
};

void DrKonqi::shutdownSaveReport()
{
    auto btGenerator = instance()->debuggerManager()->backtraceGenerator();
    auto shutdownHelper = new ShutdownHelper();
    QObject::connect(btGenerator, &BacktraceGenerator::done, shutdownHelper, &ShutdownHelper::saveReportAndQuit);
    QObject::connect(btGenerator, &BacktraceGenerator::someError, shutdownHelper, &ShutdownHelper::saveReportAndQuit);
    QObject::connect(btGenerator, &BacktraceGenerator::failedToStart, shutdownHelper, &ShutdownHelper::saveReportAndQuit);
    QObject::connect(btGenerator, &BacktraceGenerator::newLine, shutdownHelper, &ShutdownHelper::appendNewLine);
    btGenerator->start();
}

void DrKonqi::setSignal(int signal)
{
    instance()->m_signal = signal;
}

void DrKonqi::setAppName(const QString &appName)
{
    instance()->m_appName = appName;
}

void DrKonqi::setAppPath(const QString &appPath)
{
    instance()->m_appPath = appPath;
}

void DrKonqi::setAppVersion(const QString &appVersion)
{
    instance()->m_appVersion = appVersion;
}

void DrKonqi::setBugAddress(const QString &bugAddress)
{
    instance()->m_bugAddress = bugAddress;
}

void DrKonqi::setProgramName(const QString &programName)
{
    instance()->m_programName = programName;
}

void DrKonqi::setPid(int pid)
{
    instance()->m_pid = pid;
}

void DrKonqi::setKdeinit(bool kdeinit)
{
    instance()->m_kdeinit = kdeinit;
}

void DrKonqi::setSafer(bool safer)
{
    instance()->m_safer = safer;
}

void DrKonqi::setRestarted(bool restarted)
{
    instance()->m_restarted = restarted;
}

void DrKonqi::setKeepRunning(bool keepRunning)
{
    instance()->m_keepRunning = keepRunning;
}

void DrKonqi::setThread(int thread)
{
    instance()->m_thread = thread;
}

int DrKonqi::signal()
{
    return instance()->m_signal;
}

const QString &DrKonqi::appName()
{
    return instance()->m_appName;
}

const QString &DrKonqi::appPath()
{
    return instance()->m_appPath;
}

const QString &DrKonqi::appVersion()
{
    return instance()->m_appVersion;
}

const QString &DrKonqi::bugAddress()
{
    return instance()->m_bugAddress;
}

const QString &DrKonqi::programName()
{
    return instance()->m_programName;
}

int DrKonqi::pid()
{
    return instance()->m_pid;
}

bool DrKonqi::isKdeinit()
{
    return instance()->m_kdeinit;
}

bool DrKonqi::isSafer()
{
    return instance()->m_safer;
}

bool DrKonqi::isRestarted()
{
    return instance()->m_restarted;
}

bool DrKonqi::isKeepRunning()
{
    return instance()->m_keepRunning;
}

int DrKonqi::thread()
{
    return instance()->m_thread;
}

bool DrKonqi::ignoreQuality()
{
    static bool ignore = qEnvironmentVariableIsSet("DRKONQI_IGNORE_QUALITY") ||
            qEnvironmentVariableIsSet("DRKONQI_TEST_MODE");
    return ignore;
}

const QString &DrKonqi::kdeBugzillaURL()
{
    // WARNING: for practical reasons this cannot use the shared instance
    //   Initing the instances requires knowing the URL already, so we'd have
    //   an init loop. Use a local static instead. Otherwise we'd crash on
    //   initialization of global statics derived from our return value.
    //   Always copy into the local static and return that!
    static QString url;
    if (!url.isEmpty()) {
        return url;
    }

    url = QString::fromLocal8Bit(qgetenv("DRKONQI_KDE_BUGZILLA_URL"));
    if (!url.isEmpty()) {
        return url;
    }

    if (qEnvironmentVariableIsSet("DRKONQI_TEST_MODE")) {
        url = QStringLiteral("https://bugstest.kde.org/");
    } else {
        url = QStringLiteral("https://bugs.kde.org/");
    }

    return url;
}

#include "drkonqi.moc"
