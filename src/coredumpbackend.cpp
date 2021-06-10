/*
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "coredumpbackend.h"

#include <KCrash>
#include <QDebug>
#include <QProcess>
#include <QScopeGuard>
#include <QSettings>
#include <memory>

#include <unistd.h>

#include "crashedapplication.h"
#include "debugger.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqi_debug.h"
#include "linuxprocmapsparser.h"

// Only use signal safe API here.
//   man 7 signal-safety
static void emergencySaveFunction(int signal)
{
    // Should we crash while dealing with this crash, then make sure to remove the metadata file.
    // Otherwise the helper daemon will call us again, and again cause a crash, until the user manually removes the file.
    Q_UNUSED(signal);
    unlink(qPrintable(CoredumpBackend::metadataPath()));
}

bool CoredumpBackend::init()
{
    KCrash::setEmergencySaveFunction(emergencySaveFunction);

    Q_ASSERT(!metadataPath().isEmpty());
    Q_ASSERT_X(QFile::exists(metadataPath()), static_cast<const char *>(Q_FUNC_INFO), qUtf8Printable(metadataPath()));
    qCDebug(DRKONQI_LOG) << "loading metadata" << metadataPath();

    QSettings metadata(metadataPath(), QSettings::IniFormat);
    metadata.beginGroup(QStringLiteral("Journal"));
    const QStringList keys = metadata.allKeys();
    for (const auto &key : keys) {
        m_journalEntry.insert(key.toUtf8(), metadata.value(key).toByteArray());
    }
    // conceivably the file contains no Journal group for unknown reasons
    Q_ASSERT_X(!m_journalEntry.isEmpty(), static_cast<const char *>(Q_FUNC_INFO), qUtf8Printable(metadataPath()));

    AbstractDrKonqiBackend::init(); // calls constructCrashedApplication -> we need to have our members set before calling it

    if (crashedApplication()->pid() <= 0) {
        qCWarning(DRKONQI_LOG) << "Invalid pid specified or it wasn't found in journald.";
        return false;
    }

    return true;
}

CrashedApplication *CoredumpBackend::constructCrashedApplication()
{
    Q_ASSERT(!m_journalEntry.isEmpty());

    bool ok = false;
    // Journald timestamps are always micro seconds, coredumpd takes care to make sure this also the case for its timestamp.
    const std::chrono::microseconds timestamp(m_journalEntry["COREDUMP_TIMESTAMP"].toLong(&ok));
    Q_ASSERT(ok);
    const auto datetime = QDateTime::fromMSecsSinceEpoch(std::chrono::duration_cast<std::chrono::milliseconds>(timestamp).count());
    Q_ASSERT(ok);
    const QFileInfo executable(QString::fromUtf8(m_journalEntry["COREDUMP_EXE"]));
    const int signal = m_journalEntry["COREDUMP_SIGNAL"].toInt(&ok);
    Q_ASSERT(ok);
    const bool hasDeletedFiles = LinuxProc::hasMapsDeletedFiles(executable.path(), m_journalEntry["COREDUMP_PROC_MAPS"], LinuxProc::Check::Stat);

    Q_ASSERT_X(m_journalEntry["COREDUMP_PID"].toInt() == DrKonqi::pid(),
               static_cast<const char *>(Q_FUNC_INFO),
               qPrintable(QStringLiteral("journal: %1, drkonqi: %2").arg(QString::fromUtf8(m_journalEntry["COREDUMP_PID"]), QString::number(DrKonqi::pid()))));

    m_crashedApplication = std::make_unique<CrashedApplication>(DrKonqi::pid(),
                                                                DrKonqi::thread(),
                                                                signal,
                                                                executable,
                                                                DrKonqi::appVersion(),
                                                                BugReportAddress(DrKonqi::bugAddress()),
                                                                DrKonqi::programName(),
                                                                DrKonqi::productName(),
                                                                datetime,
                                                                DrKonqi::isRestarted(),
                                                                hasDeletedFiles);

    qCDebug(DRKONQI_LOG) << "Executable is:" << executable.absoluteFilePath();
    qCDebug(DRKONQI_LOG) << "Executable exists:" << executable.exists();

    return m_crashedApplication.get();
}

DebuggerManager *CoredumpBackend::constructDebuggerManager()
{
    const QList<Debugger> internalDebuggers = Debugger::availableInternalDebuggers(m_backendType);
    const QList<Debugger> externalDebuggers = Debugger::availableExternalDebuggers(m_backendType);

    const Debugger preferredDebugger(Debugger::findDebugger(internalDebuggers, QStringLiteral("gdb")));
    qCDebug(DRKONQI_LOG) << "Using debugger:" << preferredDebugger.codeName();

    m_debuggerManager = new DebuggerManager(preferredDebugger, externalDebuggers, this);
    return m_debuggerManager;
}

void CoredumpBackend::prepareForDebugger()
{
    if (m_preparationProc) {
        return; // Preparation in progress.
    }

    // Legacy coredumpd doesn't support debugger arguments. We'll have to actually extract the core and manually trace on it,
    // somewhat meh. When Ubuntu 20.04 either goes EOL or is deemed irrelevant enough we can remove the entire preparation
    // rigging (even in AbstractDrKonqiBackend).
    const bool needPreparation = (m_backendType == QLatin1String("coredumpd"));
    const bool alreadyPrepared = (m_coreDir != nullptr);

    if (!needPreparation || alreadyPrepared) {
        // Synthesize a signal.
        QMetaObject::invokeMethod(this, &CoredumpBackend::preparedForDebugger, Qt::QueuedConnection);
        return;
    }

    m_coreDir = std::make_unique<QTemporaryDir>(QDir::tempPath() + QStringLiteral("/kcrash-core"));
    Q_ASSERT(m_coreDir->isValid());

    const QString coreFile = m_coreDir->filePath(QStringLiteral("core"));
    m_preparationProc = std::make_unique<QProcess>();
    m_preparationProc->setProcessChannelMode(QProcess::ForwardedChannels);
    m_preparationProc->setProgram(QStringLiteral("coredumpctl"));
    m_preparationProc->setArguments({QStringLiteral("--output"), coreFile, QStringLiteral("dump"), QString::number(m_crashedApplication->pid())});
    QObject::connect(
        m_preparationProc.get(),
        &QProcess::finished,
        this,
        [this, coreFile](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "Coredumpd core dumping completed" << exitCode << exitStatus << coreFile;
            // We dont really care if the dumping failed. The debugger will fail if the core file isn't there and on the UI
            // side there's not much point differentiating as the user won't be able to file a bug in any case.
            m_preparationProc = nullptr;
            m_crashedApplication->m_coreFile = coreFile;
            Q_EMIT preparedForDebugger();
        },
        Qt::QueuedConnection /* queue so we don't delete the object out from under it */);
    m_preparationProc->start();
}
