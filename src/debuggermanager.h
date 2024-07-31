/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DEBUGGERMANAGER_H
#define DEBUGGERMANAGER_H

#include <QObject>

class BacktraceGenerator;
class Debugger;
class AbstractDrKonqiBackend;

class DebuggerManager : public QObject
{
    Q_OBJECT
public:
    explicit DebuggerManager(const Debugger &internalDebugger, AbstractDrKonqiBackend *backendParent);
    ~DebuggerManager() override;

    bool debuggerIsRunning() const;
    BacktraceGenerator *backtraceGenerator() const;

Q_SIGNALS:
    void debuggerStarting();
    void debuggerFinished();
    void debuggerRunning(bool running);

private Q_SLOTS:
    void onDebuggerStarting();
    void onDebuggerFinished();

private:
    struct Private;
    Private *const d;
};

#endif // DEBUGGERMANAGER_H
