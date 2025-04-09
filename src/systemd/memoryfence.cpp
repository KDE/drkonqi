// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "memoryfence.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QObject>

#include <KMemoryInfo>

#include "drkonqi_debug.h"

#include "managerinterface.h"
#include "propertiesinterface.h"
#include "unitinterface.h"

using namespace Qt::StringLiterals;

void MemoryFence::surroundMe()
{
    registerDBusTypes();
    getUnit();
}

MemoryFence::Size MemoryFence::size() const
{
    return m_size;
}

bool MemoryFence::registerDBusTypes()
{
    static const bool registered = [] {
        qDBusRegisterMetaType<QVariantMultiItem>();
        qDBusRegisterMetaType<QVariantMultiMap>();
        return true;
    }();
    return registered;
}

void MemoryFence::getUnit()
{
    if (!m_unitPath.isEmpty()) {
        getMemory();
        return;
    }

    OrgFreedesktopSystemd1ManagerInterface systemd{s_service, QStringLiteral("/org/freedesktop/systemd1"), m_bus};
    auto watcher = new QDBusPendingCallWatcher(systemd.GetUnitByPID(0U), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DRKONQI_LOG) << "Failed to get unit by pid" << reply.error();
            Q_EMIT loaded();
            return;
        }
        m_unitPath = qdbus_cast<QDBusObjectPath>(reply.argumentAt(0)).path();

        getMemory();
    });
}

std::optional<qulonglong> MemoryFence::freeRAM()
{
    KMemoryInfo info;
    if (info.isNull()) {
        return {};
    }
    return info.freePhysical() + info.buffers() + info.cached();
}

void MemoryFence::getMemory()
{
    Q_ASSERT(!m_unitPath.isEmpty());

    OrgFreedesktopDBusPropertiesInterface properties{s_service, m_unitPath, m_bus};
    auto watcher = new QDBusPendingCallWatcher(properties.GetAll(u"org.freedesktop.systemd1.Service"_s), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DRKONQI_LOG) << "Failed to get properties" << reply.error();
            Q_EMIT loaded();
            return;
        }

        const auto properties = qdbus_cast<QVariantMap>(reply.argumentAt(0));

        auto memoryCurrent = properties.value(u"MemoryCurrent"_s).toULongLong();
        constexpr auto MiB = 1024 * 1024;
        constexpr auto fallbackMemoryCurrent = 512ULL * MiB;
        memoryCurrent = std::max(memoryCurrent, fallbackMemoryCurrent);

        const auto memoryAvailable = freeRAM();

        if (!memoryAvailable) {
            // Can't set up a fence if we don't know how much memory is available.
            Q_EMIT loaded();
            return;
        }

        applyProperties(memoryCurrent, memoryAvailable.value());
    });
}

void MemoryFence::applyProperties(qulonglong memoryCurrent, qulonglong memoryAvailable)
{
    Q_ASSERT(!m_unitPath.isEmpty());

    constexpr auto GiB = 1024 * 1024 * 1024;
    if (memoryAvailable > GiB) {
        // Give the system some breathing room if possible.
        memoryAvailable = memoryAvailable - GiB;
    }
    const auto memoryAvailableGiB = memoryAvailable / GiB;
    qWarning() << "Available memory (GiB):" << memoryAvailableGiB;

    constexpr auto lotsGiB = 12;
    constexpr auto someGiB = 4;
    constexpr auto littleGiB = 2;
    if (memoryAvailableGiB > lotsGiB) {
        qCDebug(DRKONQI_LOG) << "Memory leeway is lots. We should be good";
        m_size = Size::Spacious;
    } else if (memoryAvailableGiB > someGiB) {
        qCDebug(DRKONQI_LOG) << "Memory leeway is some. We should run with constrained features set";
        m_size = Size::Some;
    } else if (memoryAvailableGiB > littleGiB) {
        qCDebug(DRKONQI_LOG) << "Memory leeway is little. We should run with even more constrained features set";
        m_size = Size::Little;
    } else {
        qCDebug(DRKONQI_LOG) << "Memory leeway is cramped. We may not be able to produce a trace";
        m_size = Size::Cramped; // In cramped mode we get by with less than one 1GiB at the cost of trace usefulness (sentry picks up the slack)
    }
    Q_EMIT sizeChanged();
    const qulonglong memoryHigh = memoryCurrent + memoryAvailable;

    OrgFreedesktopSystemd1UnitInterface unit{s_service, m_unitPath, m_bus};
    auto setProperties = QDBusMessage::createMethodCall(s_service, m_unitPath, s_unit, u"SetProperties"_s);
    setProperties << /* runtime= */ true
                  << QVariant::fromValue(QVariantMultiMap{
                         QVariantMultiItem{.key = u"MemoryHigh"_s, .value = memoryHigh},
                     });

    auto watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(setProperties), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            qCWarning(DRKONQI_LOG) << "Failed to set properties" << reply.error();
            // don't return, it's inconsequential at this point
        }
        Q_EMIT loaded();
    });
}
