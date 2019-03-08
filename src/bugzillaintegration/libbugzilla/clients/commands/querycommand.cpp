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

#include "querycommand.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>

namespace Bugzilla {

QUrlQuery QueryCommand::toQuery() const
{
    QUrlQuery query;
    return expandQuery(query, QSet<QString>());
}

QUrlQuery QueryCommand::expandQuery(QUrlQuery &query, const QSet<QString> &seen) const
{
    const auto propertyCount = metaObject()->propertyCount();
    for (int i = 0; i < propertyCount; ++i) {
        const auto property = metaObject()->property(i);
        const auto name = QString::fromLatin1(property.name());
        const auto value = property.read(this);

        if (query.hasQueryItem(name) || seen.contains(name) || name == QLatin1Literal("objectName")) {
            // The element was manually set or builtin property.
            continue;
        }

        if (value.toLongLong() < 0) {
            // Invalid value => member was not set!
            // This does generally also work for all integers, ulonglong of
            // course being the only one that can cause trouble.
            continue;
        }

        // Lists must be serialized manually. They could have a number of representations.
        Q_ASSERT_X(value.type() != QVariant::StringList, Q_FUNC_INFO,
                   qPrintable(QStringLiteral("Trying to auto serialize string list %1").arg(name)));

        // Either can't serialize or not set.
        if (value.toString().isEmpty()) {
            continue;
        }

        query.addQueryItem(name, property.read(this).toString());
    }

    return query;
}

} // namespace Bugzilla

