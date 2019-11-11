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

#include "connection.h"

#include <QUrlQuery>

#include <KIOCore/KIO/TransferJob>

#include "bugzilla_debug.h"

namespace Bugzilla {

// Static container for global default connection.
// We need a container here because the connection may be anything derived from
// Connection and its effective type may change (e.g. in autotests).
class GlobalConnection
{
public:
    ~GlobalConnection()
    {
        delete m_connection;
    }

    Connection *m_connection = new HTTPConnection;
};

Q_GLOBAL_STATIC(GlobalConnection, s_connection)

Connection &connection()
{
    return *(s_connection->m_connection);
}

void setConnection(Connection *newConnection)
{
    delete s_connection->m_connection;
    s_connection->m_connection = newConnection;
}

HTTPConnection::HTTPConnection(const QUrl &root, QObject *parent)
    : Connection(parent)
    , m_root(root)
{
}

HTTPConnection::~HTTPConnection()
{
}

void HTTPConnection::setToken(const QString &authToken)
{
    m_token = authToken;
}

APIJob *HTTPConnection::get(const QString &path, const QUrlQuery &query) const
{
    qCDebug(BUGZILLA_LOG) << path << query.toString();
    auto job = new TransferAPIJob(KIO::get(url(path, query), KIO::Reload, KIO::HideProgressInfo));
    return job;
}

APIJob *HTTPConnection::post(const QString &path, const QByteArray &data, const QUrlQuery &query) const
{
    qCDebug(BUGZILLA_LOG) << path << query.toString();
    auto job = new TransferAPIJob(KIO::http_post(url(path, query), data, KIO::HideProgressInfo));
    return job;
}

APIJob *HTTPConnection::put(const QString &path, const QByteArray &data, const QUrlQuery &query) const
{
    qCDebug(BUGZILLA_LOG) << path << query.toString();
    auto job = new TransferAPIJob(KIO::put(url(path, query), KIO::HideProgressInfo));
    job->setPutData(data);
    return job;
}

QUrl HTTPConnection::root() const
{
    return m_root;
}

QUrl HTTPConnection::url(const QString &appendix, QUrlQuery query) const
{
    QUrl url(m_root);
    url.setPath(m_root.path() + appendix);

    if (!m_token.isEmpty()) {
        query.addQueryItem(QStringLiteral("token"), m_token);
    }

    // https://bugs.kde.org/show_bug.cgi?id=413920
    // Force encoding. QUrlQuery by default wouldn't encode '+' and bugzilla doesn't like that...
    // For any query argument. Tested with username, password, and products (for bug search)
    // on bugzilla 5.0.6. As a result let's force full encoding on every argument.
    QUrlQuery escapedQuery(query); // copy delimiter properties and the like
    escapedQuery.clear(); // but then throw away the values
    for (const auto &pair : query.queryItems(QUrl::FullyDecoded)) {
        escapedQuery.addQueryItem(pair.first, QString::fromUtf8(QUrl::toPercentEncoding(pair.second)));
    }

    url.setQuery(escapedQuery);
    return url;
}

} // namespace Bugzilla

