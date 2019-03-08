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

#include "comment.h"

#include <QVariantHash>

namespace Bugzilla {

Comment::Comment(const QVariantHash &object, QObject *parent)
    : QObject(parent)
{
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        setProperty(qPrintable(it.key()), it.value());
    }
}

int Comment::bug_id() const
{
    return m_bug_id;
}

void Comment::setBug_id(int bug_id)
{
    m_bug_id = bug_id;
}

QString Comment::text() const
{
    return m_text;
}

void Comment::setText(const QString &text)
{
    m_text = text;
}

} // namespace Bugzilla
