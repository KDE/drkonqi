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

#include "bugzilla.h"

namespace Bugzilla {

QString version(KJob *kjob)
{
    const APIJob *job = qobject_cast<APIJob *>(kjob);
    const QString version = job->object().value(QLatin1String("version")).toString();
    return version;
}

APIJob *version(const Connection &connection)
{
    return connection.get(QStringLiteral("/version"));
}

LoginDetails login(KJob *kjob)
{
    const APIJob *job = qobject_cast<APIJob *>(kjob);
    const auto obj = job->object();
    const QString token = obj.value(QLatin1String("token")).toString();
    const int id = obj.value(QLatin1String("id")).toInt(-1);
    return LoginDetails { id, token };
}

APIJob *login(const QString &username, const QString &password, const Connection &connection)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("login"), username);
    query.addQueryItem(QStringLiteral("password"), password);
    query.addQueryItem(QStringLiteral("restrict_login"), QStringLiteral("true"));
    return connection.get(QStringLiteral("/login"), query);
}

} // namespace Bugzilla

