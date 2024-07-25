/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <QString>

#include <KSharedConfig>

class Debugger
{
public:
    static QList<Debugger> availableInternalDebuggers(const QString &backend);

    /** Returns true if this Debugger instance is valid, or false otherwise.
     * Debugger instances are valid only if they have been constructed from
     * availableInternalDebuggers(). If they
     * have been constructed directly using the Debugger constructor, they are invalid.
     */
    bool isValid() const;

    /** Returns true if this debugger is installed. This is determined by
     * looking for the executable that tryExec() returns. If it is in $PATH,
     * this method returns true.
     */
    bool isInstalled() const;

    /** Returns the translatable name of the debugger (eg. "GDB") */
    QString displayName() const;

    /** Returns the code name of the debugger (eg. "gdb"). */
    QString codeName() const;

    /** Returns the executable name that drkonqi should check if it exists
     * to determine whether the debugger is installed
     */
    QString tryExec() const;

    /** Returns the command that should be run to use the debugger */
    QString command() const;

    /// Supports dynamic symbol resolution
    bool supportsCommandWithSymbolResolution() const;

    /** Returns the command that should be run to use the debugger with symbol resolution enabled */
    QString commandWithSymbolResolution() const;

    /** Returns the commands that should be given to the debugger when
     * run in batch mode in order to generate a backtrace
     */
    QString backtraceBatchCommands() const;

    /** Returns the commands that should be given to the debugger before
     * getting the backtrace
     */
    QString preambleCommands() const;

    [[nodiscard]] QString execInputFile() const;

    enum ExpandStringUsage {
        ExpansionUsagePlainText,
        ExpansionUsageShell,
    };

    static void
    expandString(QString &str, ExpandStringUsage usage = ExpansionUsagePlainText, const QString &tempFile = QString(), const QString &preambleFile = QString());

    static Debugger findDebugger(const QList<Debugger> &debuggers, const QString &defaultDebuggerCodeName);

private:
    Debugger() = default;

    struct BackendData {
        QString command;
        bool supportsCommandWithSymbolResolution;
        QString commandWithSymbolResolution;
        QString backtraceBatchCommands;
        QString preambleCommands;
        // FIXME this is only used by lldb and wholly pointless because lldb supports better interaction systems
        QString execInputFile;
    };

    struct Data {
        QString displayName;
        // FIXME it's possible that we don't need this anymore once loading from ini is gone. it may only be used to resolve paths
        QString codeName;
        QString tryExec;
        std::optional<BackendData> backendData;
    };
    std::shared_ptr<Data> m_data;
    Debugger(const std::shared_ptr<Data> &data);
};

#endif
