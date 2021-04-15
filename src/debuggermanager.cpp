/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "debuggermanager.h"

#include <KConfigGroup>

#include "backtracegenerator.h"
#include "debugger.h"
#include "debuggerlaunchers.h"
#include "drkonqibackends.h"

struct DebuggerManager::Private {
    BacktraceGenerator *btGenerator = nullptr;
    bool debuggerRunning = false;
    QList<AbstractDebuggerLauncher *> externalDebuggers;
    DBusInterfaceAdaptor *dbusInterfaceAdaptor = nullptr;
};

DebuggerManager::DebuggerManager(const Debugger &internalDebugger, const QList<Debugger> &externalDebuggers, AbstractDrKonqiBackend *backendParent)
    : QObject(backendParent)
    , d(new Private)
{
    d->btGenerator = new BacktraceGenerator(internalDebugger, this);
    connect(d->btGenerator, &BacktraceGenerator::starting, this, &DebuggerManager::onDebuggerStarting);
    connect(d->btGenerator, &BacktraceGenerator::done, this, &DebuggerManager::onDebuggerFinished);
    connect(d->btGenerator, &BacktraceGenerator::someError, this, &DebuggerManager::onDebuggerFinished);
    connect(d->btGenerator, &BacktraceGenerator::failedToStart, this, &DebuggerManager::onDebuggerFinished);
    connect(d->btGenerator, &BacktraceGenerator::preparing, backendParent, &AbstractDrKonqiBackend::prepareForDebugger);
    connect(backendParent, &AbstractDrKonqiBackend::preparedForDebugger, d->btGenerator, &BacktraceGenerator::setBackendPrepared);

    for (const Debugger &debugger : std::as_const(externalDebuggers)) {
        if (debugger.isInstalled()) {
            // TODO: use TerminalDebuggerLauncher instead
            addDebugger(new DefaultDebuggerLauncher(debugger, this));
        }
    }

    // setup kdevelop compatibility
    d->dbusInterfaceAdaptor = new DBusInterfaceAdaptor(this);
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

bool DebuggerManager::showExternalDebuggers() const
{
    KConfigGroup config(KSharedConfig::openConfig(), "DrKonqi");
    return config.readEntry("ShowDebugButton", false);
}

QList<AbstractDebuggerLauncher *> DebuggerManager::availableExternalDebuggers() const
{
    return d->externalDebuggers;
}

BacktraceGenerator *DebuggerManager::backtraceGenerator() const
{
    return d->btGenerator;
}

void DebuggerManager::addDebugger(AbstractDebuggerLauncher *launcher, bool emitsignal)
{
    d->externalDebuggers.append(launcher);
    connect(launcher, &AbstractDebuggerLauncher::starting, this, &DebuggerManager::onDebuggerStarting);
    connect(launcher, &AbstractDebuggerLauncher::finished, this, &DebuggerManager::onDebuggerFinished);
    connect(launcher, &AbstractDebuggerLauncher::invalidated, this, &DebuggerManager::onDebuggerInvalidated);
    if (emitsignal) {
        emit externalDebuggerAdded(launcher);
    }
}

void DebuggerManager::onDebuggerStarting()
{
    d->debuggerRunning = true;
    emit debuggerStarting();
    emit debuggerRunning(true);
}

void DebuggerManager::onDebuggerFinished()
{
    d->debuggerRunning = false;
    emit debuggerFinished();
    emit debuggerRunning(false);
}

void DebuggerManager::onDebuggerInvalidated()
{
    auto *launcher = qobject_cast<AbstractDebuggerLauncher *>(sender());
    Q_ASSERT(launcher);
    int index = d->externalDebuggers.indexOf(launcher);
    Q_ASSERT(index >= 0);
    d->externalDebuggers.removeAt(index);
    emit externalDebuggerRemoved(launcher);
}
