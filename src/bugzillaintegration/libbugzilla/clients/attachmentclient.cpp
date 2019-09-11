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

#include "attachmentclient.h"

#include <QVariant>

namespace Bugzilla {

QList<int> AttachmentClient::createAttachment(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);

    auto ary = job->object().value(QStringLiteral("ids")).toArray();
    // It's unclear if this can happen. When the ids would be empty there was
    // an error, and when there was an error the API should have sent an error.
    Q_ASSERT(ary.size() > 0);

    QList<int> list;
    for (auto ids : ary) {
        bool ok = false;
        list.append(ids.toVariant().toInt(&ok));
        Q_ASSERT(ok);
    }

    return list;
}

KJob *AttachmentClient::createAttachment(int bugId, const NewAttachment &attachment)
{
    return m_connection.post(QStringLiteral("/bug/%1/attachment").arg(bugId),
                             attachment.toJson());
}

} // namespace Bugzilla