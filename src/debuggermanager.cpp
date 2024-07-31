/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "debuggermanager.h"

#include <KConfigGroup>

#include "backtracegenerator.h"
#include "debugger.h"
#include "drkonqibackends.h"

struct DebuggerManager::Private {
    BacktraceGenerator *btGenerator = nullptr;
    bool debuggerRunning = false;
};

DebuggerManager::DebuggerManager(const Debugger &internalDebugger, AbstractDrKonqiBackend *backendParent)
    : QObject(backendParent)
    , d(new Private)
{
    d->btGenerator = new BacktraceGenerator(internalDebugger, this);
    connect(d->btGenerator, &BacktraceGenerator::starting, this, &DebuggerManager::onDebuggerStarting);
    connect(d->btGenerator, &BacktraceGenerator::done, this, &DebuggerManager::onDebuggerFinished);
    connect(d->btGenerator, &BacktraceGenerator::someError, this, &DebuggerManager::onDebuggerFinished);
    connect(d->btGenerator, &BacktraceGenerator::failedToStart, this, &DebuggerManager::onDebuggerFinished);
    connect(d->btGenerator, &BacktraceGenerator::preparing, backendParent, &AbstractDrKonqiBackend::prepareForDebugger);
    connect(backendParent, &AbstractDrKonqiBackend::failedToPrepare, d->btGenerator, &BacktraceGenerator::setBackendFailed);
    connect(backendParent, &AbstractDrKonqiBackend::preparedForDebugger, d->btGenerator, &BacktraceGenerator::setBackendPrepared);
}

DebuggerManager::~DebuggerManager()
{
    if (d->btGenerator->state() == BacktraceGenerator::Loading) {
        // if the debugger is running, kill it and continue the process.
        delete d->btGenerator;
        onDebuggerFinished();
    }

    delete d;
}

bool DebuggerManager::debuggerIsRunning() const
{
    return d->debuggerRunning;
}

BacktraceGenerator *DebuggerManager::backtraceGenerator() const
{
    return d->btGenerator;
}

void DebuggerManager::onDebuggerStarting()
{
    d->debuggerRunning = true;
    Q_EMIT debuggerStarting();
    Q_EMIT debuggerRunning(true);
}

void DebuggerManager::onDebuggerFinished()
{
    d->debuggerRunning = false;
    Q_EMIT debuggerFinished();
    Q_EMIT debuggerRunning(false);
}

#include "moc_debuggermanager.cpp"
