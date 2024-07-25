/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "debugger.h"

#include <KConfig>
#include <KConfigGroup>
#include <KFileUtils>
#include <KLocalizedString>
#include <KMacroExpander>
#include <QCoreApplication>
#include <QDir>

#include "crashedapplication.h"
#include "drkonqi.h"
#include "drkonqi_debug.h"

using namespace Qt::StringLiterals;

// static
QList<Debugger> Debugger::availableInternalDebuggers(const QString &backend)
{
    QList<Debugger> result;

    const auto expandCommand = [](const QString &codeName, const QString &command) {
        static QHash<QString, QString> map = {
            {QStringLiteral("drkonqi_datadir"), QStandardPaths::locate(QStandardPaths::AppDataLocation, codeName, QStandardPaths::LocateDirectory)},
        };
        return KMacroExpander::expandMacros(command, map);
    };

    if (backend == "KCrash"_L1) {
        result.push_back(std::make_shared<Data>(
            Data{.displayName = i18nc("@label the debugger called GDB", "GDB"),
                 .codeName = u"gdb"_s,
                 .tryExec = u"gdb"_s,
                 .backendData =
                     BackendData{.command = u"gdb -nw -n -batch -x %preamblefile -x %tempfile -p %pid %execpath"_s,
                                 .supportsCommandWithSymbolResolution = true,
                                 .commandWithSymbolResolution =
                                     u"gdb -nw -n -batch --init-eval-command='set debuginfod enabled on' -x %preamblefile -x %tempfile -p %pid %execpath"_s,
                                 .backtraceBatchCommands = u"thread\nthread apply all bt"_s,
                                 .preambleCommands = expandCommand(
                                     u"gdb"_s,
                                     u"set width 200\nset backtrace limit 128\nsource %drkonqi_datadir/python/gdb_preamble/preamble.py\npy print_preamble()"_s),
                                 .execInputFile = {}}}));

        result.push_back(std::make_shared<Data>( //
            Data{.displayName = i18nc("@label the debugger called LLDB", "LLDB"),
                 .codeName = u"lldb"_s,
                 .tryExec = u"lldb"_s,
                 .backendData = BackendData{.command = u"lldb -p %pid"_s,
                                            .supportsCommandWithSymbolResolution = false,
                                            .commandWithSymbolResolution = {},
                                            .backtraceBatchCommands = u"settings set term-width 200\nthread info\nbt all"_s,
                                            .preambleCommands = {},
                                            .execInputFile = u"%tempfile"_s}}));
    } else if (backend == "coredump-core"_L1) {
        result.push_back(std::make_shared<Data>( //
            Data{
                .displayName = i18nc("@label the debugger called GDB", "GDB"),
                .codeName = u"gdb"_s,
                .tryExec = u"gdb"_s,
                .backendData = BackendData{
                    .command = u"gdb --nw --nx --batch --command=%preamblefile --command=%tempfile --core=%corefile %execpath"_s,
                    .supportsCommandWithSymbolResolution = true,
                    .commandWithSymbolResolution =
                        u"gdb --nw --nx --batch --init-eval-command='set debuginfod enabled on' --command=%preamblefile --command=%tempfile --core=%corefile %execpath"_s,
                    .backtraceBatchCommands = u"thread\nthread apply all bt"_s,
                    .preambleCommands = expandCommand(
                        u"gdb"_s,
                        u"set width 200\nset backtrace limit 128\nsource %drkonqi_datadir/python/gdb_preamble/preamble.py\npy print_preamble()"_s),
                    .execInputFile = {}}}));
    }

    return result;
}

bool Debugger::isValid() const
{
    return m_data && m_data->backendData.has_value() && (!tryExec().isEmpty() || !displayName().isEmpty());
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

QString Debugger::execInputFile() const
{
    return m_data->backendData->execInputFile;
}

Debugger::Debugger(const std::shared_ptr<Data> &data)
    : m_data(data)
{
}
