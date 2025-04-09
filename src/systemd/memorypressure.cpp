// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "memorypressure.h"

#include <sys/sysinfo.h>
#include <systemd/sd-event.h>

#include <QCoreApplication>
#include <QFile>
#include <QMutex>
#include <QObject>
#include <QSocketNotifier>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

#include <KDirWatch>

#include "safe_strerror.h"

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

// Context container living inside the pressure monitor thread.
// Only interact with it via signals so Qt automatically handles the thread queuing.
class SDPressureContext : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

public Q_SLOTS:
    void onMonitor(pid_t pid)
    {
        m_pids << pid;
    }

    void onPidReset()
    {
        m_pids.clear();
    }

public:
    QList<pid_t> m_pids;
};

// If we are under memory pressure and the main thread gets stuck on allocations inside Qt or glib we haven't won anything.
// Instead run an sd-event loop in a thread and hope it's lightweight nature will be less susceptible to memory problems.
class SDPressureMonitor : public QThread
{
    Q_OBJECT
public:
    SDPressureMonitor(MemoryPressure *pressure, QObject *parent = nullptr)
        : QThread(parent)
        , m_context([this, &pressure] {
            auto context = new SDPressureContext(nullptr /* gets moveToThread */);
            context->moveToThread(this);
            connect(pressure, &MemoryPressure::monitoring, context, &SDPressureContext::onMonitor);
            connect(pressure, &MemoryPressure::pidReset, context, &SDPressureContext::onPidReset);
            return context;
        }())
    {
        setPriority(QThread::HighPriority);
    }

    ~SDPressureMonitor() override
    {
        // In case the thread gets terminated before its natural end.
        if (m_loop) {
            sd_event_exit(m_loop, 0);
            sd_event_unref(m_loop);
        }
    }
    Q_DISABLE_COPY_MOVE(SDPressureMonitor)

public Q_SLOTS:
    void exitEventLoop()
    {
        QMutexLocker locker(&m_mutex);
        pthread_kill(m_tid, SIGUSR1);
        m_quitCondition.wait(&m_mutex, 250ms);
        sd_event_unref(m_loop);
        m_loop = nullptr;
    }

protected:
    void run() override
    {
        QMutexLocker locker(&m_mutex);

        m_tid = pthread_self();

        if (auto ret = sd_event_default(&m_loop); ret < 0) {
            qWarning() << "Failed to create event loop" << safe_strerror(-ret);
            return;
        }

        if (auto ret = sd_event_add_memory_pressure(
                m_loop,
                nullptr,
                []([[maybe_unused]] sd_event_source *source, void *userdata) {
                    // DO NOT LOG BEFORE THE KILL! It may allocate memory and itself get stuck!
                    auto that = static_cast<SDPressureMonitor *>(userdata);
                    that->onPressure();
                    return 0;
                },
                this);
            ret < 0) {
            qWarning() << "Failed to add memory pressure event source" << safe_strerror(-ret);
            return;
        }

        sd_event_add_signal(
            m_loop,
            nullptr,
            SIGUSR1 | SD_EVENT_SIGNAL_PROCMASK,
            [](sd_event_source *s, [[maybe_unused]] const signalfd_siginfo *siginfo, [[maybe_unused]] void *userdata) {
                sd_event_exit(sd_event_source_get_event(s), 0);
                return 0;
            },
            nullptr);

        sd_event_add_defer(
            m_loop,
            nullptr,
            []([[maybe_unused]] sd_event_source *s, void *userdata) {
                // We are inside the loop now. We can give up the lock.
                static_cast<QMutexLocker<QMutex> *>(userdata)->unlock();
                return 0;
            },
            &locker);

        sd_event_loop(m_loop);
        QMutexLocker locker2(&m_mutex);
        m_quitCondition.wakeAll();
    }

Q_SIGNALS:
    void underPressure();

private:
    void onPressure()
    {
        // DO NOT LOG BEFORE THE KILL! It may allocate memory and itself get stuck!
        // Also do not detach m_pids. Same reason.
        for (const auto &pid : std::as_const(m_context->m_pids)) {
            kill(pid, SIGKILL);
        }
        qDebug() << "pressure high. killed" << m_context->m_pids;
        m_context->m_pids.clear();
        Q_EMIT underPressure();
    }

    SDPressureContext *m_context;

    ///
    QMutex m_mutex;
    pthread_t m_tid = 0;
    sd_event *m_loop = nullptr;
    QWaitCondition m_quitCondition;
    ///
};

MemoryPressure::~MemoryPressure()
{
    if (m_monitor) {
        // Negotiate with the thread to exit the event loop.
        m_monitor->exitEventLoop();
        m_monitor->quit();
        m_monitor->wait(250ms);
        m_monitor->terminate();
        m_monitor->wait(250ms);
        m_monitor->deleteLater();
    }
}

void MemoryPressure::reset()
{
    m_level = Level::Low;
    Q_EMIT levelChanged();
}

MemoryPressure::Level MemoryPressure::level() const
{
    return m_level;
}

MemoryPressure::MemoryPressure(QObject *parent)
    : QObject(parent)
    , m_monitor([this] {
        auto monitor = new SDPressureMonitor(this, /* parent = */ nullptr) /* parentless gets deleted manually after quit! */;
        connect(monitor, &SDPressureMonitor::underPressure, this, [this] {
            m_level = Level::High;
            Q_EMIT levelChanged();
        });
        monitor->start();
        return monitor;
    }())
{
    reset();
}

MemoryPressure *MemoryPressure::instance()
{
    static MemoryPressure instance;
    return &instance;
}

#include "memorypressure.moc"
