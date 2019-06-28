/*
    Copyright 2019 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BUG_H
#define BUG_H

#include <QObject>
#include <QPointer>

#include <QVariant>

#include "comment.h"

namespace Bugzilla {

// Models a bugzilla bug.
class Bug : public QObject
{
public:
    enum class Status {
        Unknown, // First value is default if QMetaEnum can't map the key.
        UNCONFIRMED,
        CONFIRMED,
        ASSIGNED,
        REOPENED,
        RESOLVED,
        NEEDSINFO,
        VERIFIED,
        CLOSED
    };
    Q_ENUM(Status)

    enum class Resolution {
        Unknown, // First value is default if QMetaEnum can't map the key.
        NONE, // Fake value, expresses unresoled. On the REST side this is an empty string.
        FIXED,
        INVALID,
        WONTFIX,
        LATER,
        REMIND,
        DUPLICATE,
        WORKSFORME,
        MOVED,
        UPSTREAM,
        DOWNSTREAM,
        WAITINGFORINFO,
        BACKTRACE,
        UNMAINTAINED
    };
    Q_ENUM(Resolution)

private:
    Q_OBJECT
    Q_PROPERTY(qint64 id READ id WRITE setId)
    Q_PROPERTY(QString product READ product WRITE setProduct)
    Q_PROPERTY(QString component READ component WRITE setComponent)
    Q_PROPERTY(QString summary READ summary WRITE setSummary)
    Q_PROPERTY(QString version READ version WRITE setVersion)
    Q_PROPERTY(bool is_open READ is_open WRITE setIs_open)
    // maybe should be camel mapped, who knows
    Q_PROPERTY(QString op_sys READ op_sys WRITE setOp_sys)
    Q_PROPERTY(QString priority READ priority WRITE setPriority)
    Q_PROPERTY(QString severity READ severity WRITE setSeverity)
    Q_PROPERTY(Status status READ status WRITE setStatus)
    Q_PROPERTY(Resolution resolution READ resolution WRITE setResolution)
    Q_PROPERTY(qint64 dupe_of READ dupe_of WRITE setDupe_of)

    // Custom fields (versionfixedin etc) are only available via customField().

public:
    typedef QPointer<Bug> Ptr;

    explicit Bug(const QVariantHash &object, QObject *parent = nullptr);

    qint64 id() const;
    void setId(qint64 id);

    QVariant customField(const char *key);

    Status status() const;
    void setStatus(Status status);

    Resolution resolution() const;
    void setResolution(Resolution resolution);

    QString summary() const;
    void setSummary(const QString &summary);

    QString version() const;
    void setVersion(const QString &version);

    QString product() const;
    void setProduct(const QString &product);

    QString component() const;
    void setComponent(const QString &component);

    QString op_sys() const;
    void setOp_sys(const QString &op_sys);

    QString priority() const;
    void setPriority(const QString &priority);

    QString severity() const;
    void setSeverity(const QString &severity);

    bool is_open() const;
    void setIs_open(bool is_open);

    qint64 dupe_of() const;
    void setDupe_of(qint64 dupe_of);

Q_SIGNALS:
    void commentsChanged();

private:
    qint64 m_id = -1;
    QString m_product;
    QString m_component;
    QString m_summary;
    QString m_version;
    bool m_is_open = false;
    QString m_op_sys;
    QString m_priority;
    QString m_severity;
    Status m_status = Status::Unknown;
    Resolution m_resolution = Resolution::Unknown;
    qint64 m_dupe_of = -1;
};

} // namespace Bugzilla

#endif // BUG_H
