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

#include "commentclient.h"

#include <QVariantHash>

namespace Bugzilla {

QList<Comment::Ptr> CommentClient::getFromBug(KJob *kjob)
{
    APIJob *job = qobject_cast<APIJob *>(kjob);
    QJsonObject bugs = job->object().value(QStringLiteral("bugs")).toObject();

    // The API should never return anything other than the single bug we asked for.
    Q_ASSERT(bugs.keys().size() == 1);

    QJsonObject bug = bugs.value(bugs.keys().at(0)).toObject();
    QJsonArray comments = bug.value(QStringLiteral("comments")).toArray();

    QList<Comment::Ptr> list;
    for (auto it = comments.constBegin(); it != comments.constEnd(); ++it) {
        list.append(new Comment((*it).toObject().toVariantHash()));
    }

    return list;
}

KJob *CommentClient::getFromBug(int bugId)
{
    return m_connection.get(QStringLiteral("/bug/%1/comment").arg(QString::number(bugId)));
}

} // namespace Bugzilla
