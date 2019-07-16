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

#ifndef BUGFIELDCLIENT_H
#define BUGFIELDCLIENT_H

#include "clientbase.h"
#include "models/bugfield.h"

namespace Bugzilla {

class BugFieldClient : public ClientBase
{
public:
    using ClientBase::ClientBase;

    KJob *getFields(const QString &idOrName = QString());
    QList<BugField::Ptr> getFields(KJob *kjob);

    KJob *getField(const QString &idOrName);
    /// Ptr may be null if the idOrName matched nothing!
    BugField::Ptr getField(KJob *kjob);
};

} // namespace Bugzilla

#endif // BUGFIELDCLIENT_H
