/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "debugger.h"

#include <KConfig>
#include <KConfigGroup>
#include <KFileUtils>
#include <KMacroExpander>
#include <QCoreApplication>
#include <QDir>

#include "crashedapplication.h"
#include "drkonqi.h"
#include "drkonqi_debug.h"

// static
QList<Debugger> Debugger::availableInternalDebuggers(const QString &backend)
{
    return availableDebuggers(QStringLiteral("debuggers/internal"), backend);
}

bool Debugger::isValid() const
{
    return (!tryExec().isEmpty() || !displayName().isEmpty()) && m_data->backendData.has_value();
}

bool Debugger::isInstalled() const
{
    QString tryexec = tryExec();
    if (tryexec.isEmpty()) {
        qCDebug(DRKONQI_LOG) << "tryExec of" << codeName() << "is empty!";
        return false;
    }

    // Find for executable in PATH and in our application path
    return !QStandardPaths::findExecutable(tryexec).isEmpty() || !QStandardPaths::findExecutable(tryexec, {QCoreApplication::applicationDirPath()}).isEmpty();
}

QString Debugger::displayName() const
{
    return m_data->displayName;
}

QString Debugger::codeName() const
{
    // fall back to the "TryExec" string if "CodeName" is not specified.
    // for most debuggers those strings should be the same
    return !m_data->codeName.isEmpty() ? m_data->codeName : m_data->tryExec;
}

QString Debugger::tryExec() const
{
    return m_data->tryExec;
}

QString Debugger::command() const
{
    return m_data->backendData->command;
}

bool Debugger::supportsCommandWithSymbolResolution() const
{
    // TODO this is a bit pointless the resolver command falls back to command so you can just always use the resolver
    return m_data->backendData->supportsCommandWithSymbolResolution;
}

QString Debugger::commandWithSymbolResolution() const
{
    return m_data->backendData->commandWithSymbolResolution;
}

QString Debugger::backtraceBatchCommands() const
{
    return m_data->backendData->backtraceBatchCommands;
}

QString Debugger::preambleCommands() const
{
    return m_data->backendData->preambleCommands;
}


void Debugger::expandString(QString &str, ExpandStringUsage usage, const QString &tempFile, const QString &preambleFile)
{
    const CrashedApplication *appInfo = DrKonqi::crashedApplication();
    const QHash<QString, QString> map = {
        {QLatin1String("progname"), appInfo->name()},
        {QLatin1String("execname"), appInfo->fakeExecutableBaseName()},
        {QLatin1String("execpath"), appInfo->executable().absoluteFilePath()},
        {QLatin1String("signum"), QString::number(appInfo->signalNumber())},
        {QLatin1String("signame"), appInfo->signalName()},
        {QLatin1String("pid"), QString::number(appInfo->pid())},
        {QLatin1String("tempfile"), tempFile},
        {QLatin1String("preamblefile"), preambleFile},
        {QLatin1String("thread"), QString::number(appInfo->thread())},
        {QStringLiteral("corefile"), appInfo->m_coreFile},
    };

    if (usage == ExpansionUsageShell) {
        str = KMacroExpander::expandMacrosShellQuote(str, map);
    } else {
        str = KMacroExpander::expandMacros(str, map);
    }
}

QList<Debugger> Debugger::availableDebuggers(const QString &path, const QString &backend)
{
    const QStringList debuggerDirs{// Search from application path, this helps when deploying an application
                                   // as binary blob (e.g. Windows exe).
                                   QCoreApplication::applicationDirPath() + QLatin1Char('/') + path,
                                   // Search in default path
                                   QStandardPaths::locate(QStandardPaths::AppDataLocation, path, QStandardPaths::LocateDirectory)};
    const QStringList debuggerFiles = KFileUtils::findAllUniqueFiles(debuggerDirs);

    QList<Debugger> result;
    for (const auto &debuggerFile : debuggerFiles) {
        Debugger debugger(KSharedConfig::openConfig(debuggerFile), backend);
        if (debugger.isValid()) {
            result << debugger;
        }
    }
    return result;
}

Debugger Debugger::findDebugger(const QList<Debugger> &debuggers, const QString &defaultDebuggerCodeName)
{
    Debugger firstKnownGoodDebugger;
    Debugger preferredDebugger;
    for (const Debugger &debugger : debuggers) {
        qCDebug(DRKONQI_LOG) << "Check debugger if" << debugger.displayName() << "[" << debugger.codeName() << "]"
                             << "is installed:" << debugger.isInstalled();
        if (!firstKnownGoodDebugger.isValid() && debugger.isInstalled()) {
            firstKnownGoodDebugger = debugger;
        }
        if (debugger.codeName() == defaultDebuggerCodeName) {
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
            qCWarning(DRKONQI_LOG) << "Unable to find an internal debugger that can work with the crash backend";
        }
    }

    return preferredDebugger;
}

std::optional<Debugger::BackendData> Debugger::loadBackendData(const KSharedConfig::Ptr &config, const QString &backend)
{
    const auto general = config->group(QStringLiteral("General"));
    const auto supportedBackends = general.readEntry("Backends").split(QLatin1Char('|'), Qt::SkipEmptyParts);

    if (!supportedBackends.contains(backend)) {
        return {};
    }

    const auto expandCommand = [codeName = general.readEntry("CodeName")](const QString &command) {
        static QHash<QString, QString> map = {
            {QStringLiteral("drkonqi_datadir"), QStandardPaths::locate(QStandardPaths::AppDataLocation, codeName, QStandardPaths::LocateDirectory)},
        };
        return KMacroExpander::expandMacros(command, map);
    };

    const auto group = config->group(backend);
    const auto command = expandCommand(group.readPathEntry("Exec", QString()));
    return BackendData{
        .command = command,
        .supportsCommandWithSymbolResolution = group.hasKey("ExecWithSymbolResolution"),
        .commandWithSymbolResolution = expandCommand(group.readPathEntry("ExecWithSymbolResolution", command)),
        .backtraceBatchCommands = expandCommand(group.readPathEntry("BatchCommands", QString())),
        .preambleCommands = expandCommand(group.readPathEntry("PreambleCommands", QString())),
        .execInputFile = group.readEntry("ExecInputFile"),
    };
}

Debugger::Debugger(const KSharedConfig::Ptr &config, const QString &backend)
    : m_data(new Data{
          .displayName = config->group(QStringLiteral("General")).readEntry("Name"),
          .codeName = config->group(QStringLiteral("General")).readEntry("CodeName"),
          .tryExec = config->group(QStringLiteral("General")).readEntry("TryExec"),
          .backendData = loadBackendData(config, backend),
      })
{
}

QString Debugger::execInputFile() const
{
    return m_data->backendData->execInputFile;
}
