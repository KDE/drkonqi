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

#include "jsoncommand.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>

namespace Bugzilla {

QByteArray JsonCommand::toJson() const
{
    QJsonDocument doc;
    doc.setObject(QJsonObject::fromVariantHash(toVariantHash()));
    return doc.toJson();
}

QVariantHash JsonCommand::toVariantHash() const
{
    QVariantHash hash;

    const auto propertyCount = metaObject()->propertyCount();
    for (int i = 0; i < propertyCount; ++i) {
        const auto property = metaObject()->property(i);
        const auto name = QString::fromLatin1(property.name());
        const auto value = property.read(this);

        if (name == QStringLiteral("objectName")) { // Builtin property.
            continue;
        }

        if (value.isNull()) {
            continue;
        }

        // If this is a nested representation, serialize it and glue it in.
        if (value.canConvert<JsonCommand *>()) {
            JsonCommand *repValue = value.value<JsonCommand *>();
            hash.insert(name, repValue->toVariantHash());
            continue;
        }

        hash.insert(name, value);
    }

    return hash;
}

} // namespace Bugzilla

