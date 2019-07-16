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

#include "bugfield.h"

namespace Bugzilla {

BugField::BugField(const QVariantHash &obj, QObject *parent)
    : QObject(parent)
{
    registerVariantConverters();

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        setProperty(qPrintable(it.key()), it.value());
    }
}

QList<BugFieldValue *> BugField::values() const
{
    return m_values;
}

void BugField::setValues(QList<BugFieldValue *> values)
{
    m_values = values;
}

void BugField::registerVariantConverters()
{
    static bool convertersRegistered = false;
    if (convertersRegistered) {
        return;
    }
    convertersRegistered = true;

    QMetaType::registerConverter<QVariantList, QList<BugFieldValue *>>(
                [](QVariantList v) -> QList<BugFieldValue *>
    {
        QList<BugFieldValue *> list;
        list.reserve(v.size());
        for (const QVariant &variant : qAsConst(v)) {
            list.append(new BugFieldValue(variant.toHash()));
        }
        return list;
    });
}

BugFieldValue::BugFieldValue(const QVariantHash &obj, QObject *parent)
    : QObject(parent)
{
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        setProperty(qPrintable(it.key()), it.value());
    }
}

QString BugFieldValue::name() const
{
    return m_name;
}

void BugFieldValue::setName(QString name)
{
    m_name = name;
}

} // namespace Bugzilla
