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

#ifndef BUGCLIENT_H
#define BUGCLIENT_H

#include "clientbase.h"
#include "commands/bugsearch.h"
#include "commands/bugupdate.h"
#include "commands/newbug.h"

#include <models/bug.h>

namespace Bugzilla {

class BugClient : public ClientBase
{
public:
    using ClientBase::ClientBase;

    QList<Bug::Ptr> search(KJob *kjob);
    KJob *search(const BugSearch &search);

    qint64 create(KJob *kjob);
    KJob *create(const NewBug &bug);

    qint64 update(KJob *kjob);
    KJob *update(qint64 bugId, BugUpdate &bug);
};

} // namespace Bugzilla

#endif // BUGCLIENT_H
