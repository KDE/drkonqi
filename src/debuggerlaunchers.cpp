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
*/
#include "debuggerlaunchers.h"

#include <QDBusConnection>

#include <KShell>
#include <KProcess>
#include "drkonqi_debug.h"

#include "detachedprocessmonitor.h"
#include "drkonqi.h"
#include "crashedapplication.h"

DefaultDebuggerLauncher::DefaultDebuggerLauncher(const Debugger & debugger, DebuggerManager *parent)
    : AbstractDebuggerLauncher(parent), m_debugger(debugger)
{
    m_monitor = new DetachedProcessMonitor(this);
    connect(m_monitor, &DetachedProcessMonitor::processFinished, this, &DefaultDebuggerLauncher::onProcessFinished);
}

QString DefaultDebuggerLauncher::name() const
{
    return m_debugger.name();
}

void DefaultDebuggerLauncher::start()
{
    if ( static_cast<DebuggerManager*>(parent())->debuggerIsRunning() ) {
        qCWarning(DRKONQI_LOG) << "Another debugger is already running";
        return;
    }

    QString str = m_debugger.command();
    Debugger::expandString(str, Debugger::ExpansionUsageShell);

    emit starting();
    int pid = KProcess::startDetached(KShell::splitArgs(str));
    if ( pid > 0 ) {
        m_monitor->startMonitoring(pid);
    } else {
        qCWarning(DRKONQI_LOG) << "Could not start debugger:" << name();
        emit finished();
    }
}

void DefaultDebuggerLauncher::onProcessFinished()
{
    emit finished();
}

#if 0
TerminalDebuggerLauncher::TerminalDebuggerLauncher(const Debugger & debugger, DebuggerManager *parent)
    : DefaultDebuggerLauncher(debugger, parent)
{
}

void TerminalDebuggerLauncher::start()
{
    DefaultDebuggerLauncher::start(); //FIXME
}
#endif


DBusInterfaceLauncher::DBusInterfaceLauncher(const QString &name, DBusInterfaceAdaptor *parent)
    : AbstractDebuggerLauncher(parent), m_name(name)
{
}

QString DBusInterfaceLauncher::name() const
{
    return m_name;
}

void DBusInterfaceLauncher::start()
{
    emit starting();
    emit static_cast<DBusInterfaceAdaptor*>(parent())->acceptDebuggingApplication(m_name);
}


DBusInterfaceAdaptor::DBusInterfaceAdaptor(DebuggerManager *parent)
    : QDBusAbstractAdaptor(parent)
{
    Q_ASSERT(parent);

    if (QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.drkonqi-%1").arg(pid()))) {
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/debugger"), parent);
    }
}

int DBusInterfaceAdaptor::pid()
{
    return DrKonqi::crashedApplication()->pid();
}

void DBusInterfaceAdaptor::registerDebuggingApplication(const QString &name)
{
    if (!name.isEmpty() && !m_launchers.contains(name)) {
        auto launcher = new DBusInterfaceLauncher(name, this);
        m_launchers.insert(name, launcher);
        static_cast<DebuggerManager*>(parent())->addDebugger(launcher, true);
    }
}

void DBusInterfaceAdaptor::debuggingFinished(const QString &name)
{
    auto it = m_launchers.find(name);
    if (it != m_launchers.end()) {
        emit it.value()->finished();
    }
}

void DBusInterfaceAdaptor::debuggerClosed(const QString &name)
{
    auto it = m_launchers.find(name);
    if (it != m_launchers.end()) {
        emit it.value()->invalidated();
    }
}
