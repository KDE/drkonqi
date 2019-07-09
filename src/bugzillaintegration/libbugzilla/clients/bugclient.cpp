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

#include "bugclient.h"


namespace Bugzilla {

QList<Bug::Ptr> BugClient::search(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);

    auto ary = job->object().value(QStringLiteral("bugs")).toArray();

    QList<Bug::Ptr> list;
    for (auto bug : ary) {
        list.append(Bug::Ptr(new Bug(bug.toObject().toVariantHash())));
    }

    return list;
}

KJob *BugClient::search(const BugSearch &search)
{
    return m_connection.get(QStringLiteral("/bug"), search.toQuery());
}

qint64 BugClient::create(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);

    qint64 ret = job->object().value(QStringLiteral("id")).toInt(-1);
    Q_ASSERT(ret != -1);
    return ret;
}

KJob *BugClient::create(const NewBug &bug)
{
    return m_connection.post(QStringLiteral("/bug"),
                             bug.toJson());
}

qint64 BugClient::update(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);

    auto ary = job->object().value(QStringLiteral("bugs")).toArray();
    // It's unclear if this can happen. When the ids would be empty there was
    // an error, and when there was an error the API should have sent an error.
    Q_ASSERT(ary.size() == 1);

    int value = ary.at(0).toObject().value(QStringLiteral("id")).toInt(-1);
    Q_ASSERT(value != -1);

    return value;
}

KJob *BugClient::update(qint64 bugId, BugUpdate &bug)
{
    return m_connection.put(QStringLiteral("/bug/%1").arg(bugId), bug.toJson());
}

} // namespace Bugzilla
