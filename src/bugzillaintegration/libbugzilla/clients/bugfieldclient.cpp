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

#include "bugfieldclient.h"

namespace Bugzilla {

KJob *BugFieldClient::getFields(const QString &idOrName)
{
    return m_connection.get(QStringLiteral("/field/bug/") + idOrName);
}

QList<BugField::Ptr> BugFieldClient::getFields(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);

    const auto ary = job->object().value(QStringLiteral("fields")).toArray();

    QList<BugField::Ptr> list;
    list.reserve(ary.size());
    for (const auto &bug : qAsConst(ary)) {
        list.append(BugField::Ptr(new BugField(bug.toObject().toVariantHash())));
    }
    return list;
}

KJob *BugFieldClient::getField(const QString &idOrName)
{
    return getFields(idOrName);
}

BugField::Ptr BugFieldClient::getField(KJob *kjob)
{
    return getFields(kjob).value(0, nullptr);
}

} // namespace Bugzilla
