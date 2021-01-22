/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "bug.h"

#include <QDebug>

namespace Bugzilla
{
Bug::Bug(const QVariantHash &obj, QObject *parent)
    : QObject(parent)
{
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        setProperty(qPrintable(it.key()), it.value());
    }

    // Enums are auto-translated from strings so long as the string is equal
    // to the stringified enum key. Fail if the mapping failed.
    const QString status = obj.value(QStringLiteral("status")).toString();
    if (m_status == Status::Unknown) {
        // Intentionally uncategorized. Very important warning!
        qWarning() << "Drkonqi status mapping failed on bug" << id() << ":" << status << "Please file a bug at bugs.kde.org";
    }

    const QString resolution = obj.value(QStringLiteral("resolution")).toString();
    if (resolution.isEmpty() && m_resolution == Resolution::Unknown) {
        m_resolution = Resolution::NONE;
        // The empty string is unresolved. This is expected and shouldn't trip
        // the mapping guard.
    } else if (m_resolution == Resolution::Unknown) {
        // Intentionally uncategorized. Very important warning!
        qWarning() << "Drkonqi resolution mapping failed on bug" << id() << ":" << resolution << "Please file a bug at bugs.kde.org";
    }
}

Bug::Resolution Bug::resolution() const
{
    return m_resolution;
}

void Bug::setResolution(Resolution resolution)
{
    m_resolution = resolution;
}

QString Bug::summary() const
{
    return m_summary;
}

void Bug::setSummary(const QString &summary)
{
    m_summary = summary;
}

QString Bug::version() const
{
    return m_version;
}

void Bug::setVersion(const QString &version)
{
    m_version = version;
}

QString Bug::product() const
{
    return m_product;
}

void Bug::setProduct(const QString &product)
{
    m_product = product;
}

QString Bug::component() const
{
    return m_component;
}

void Bug::setComponent(const QString &component)
{
    m_component = component;
}

QString Bug::op_sys() const
{
    return m_op_sys;
}

void Bug::setOp_sys(const QString &op_sys)
{
    m_op_sys = op_sys;
}

QString Bug::priority() const
{
    return m_priority;
}

void Bug::setPriority(const QString &priority)
{
    m_priority = priority;
}

QString Bug::severity() const
{
    return m_severity;
}

void Bug::setSeverity(const QString &severity)
{
    m_severity = severity;
}

bool Bug::is_open() const
{
    return m_is_open;
}

void Bug::setIs_open(bool is_open)
{
    m_is_open = is_open;
}

qint64 Bug::dupe_of() const
{
    return m_dupe_of;
}

void Bug::setDupe_of(qint64 dupe_of)
{
    m_dupe_of = dupe_of;
}

qint64 Bug::id() const
{
    return m_id;
}

void Bug::setId(qint64 id)
{
    m_id = id;
}

QVariant Bug::customField(const char *key)
{
    return property(key);
}

Bug::Status Bug::status() const
{
    return m_status;
}

void Bug::setStatus(Status status)
{
    m_status = status;
}

} // namespace Bugzilla
