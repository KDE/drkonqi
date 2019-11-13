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

#ifndef NEWBUG_H
#define NEWBUG_H

#include "jsoncommand.h"

namespace Bugzilla {

class NewBug : public JsonCommand
{
    Q_OBJECT
    BUGZILLA_MEMBER_PROPERTY(QString, product);
    BUGZILLA_MEMBER_PROPERTY(QString, component);
    BUGZILLA_MEMBER_PROPERTY(QString, summary); // aka shortdesc
    BUGZILLA_MEMBER_PROPERTY(QString, version);
    BUGZILLA_MEMBER_PROPERTY(QString, description);
    BUGZILLA_MEMBER_PROPERTY(QString, op_sys);
    BUGZILLA_MEMBER_PROPERTY(QString, platform);
    BUGZILLA_MEMBER_PROPERTY(QString, priority);
    BUGZILLA_MEMBER_PROPERTY(QString, severity);
    BUGZILLA_MEMBER_PROPERTY(QStringList, keywords); // not documented but also supported
public:
    // only needed because we impl the copy ctor, otherwise this could be `using`
    explicit NewBug(QObject *parent = nullptr);
    NewBug(const NewBug &other);
};

} // namespace Bugzilla

#endif // NEWBUG_H
