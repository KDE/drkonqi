/*
    SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "connection.h"

#include <KIOCore/KIO/TransferJob>
#include <QUrlQuery>

#include "bugzilla_debug.h"

namespace Bugzilla
{
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

APIJob *HTTPConnection::get(const QString &path, const Query &query) const
{
    qCDebug(BUGZILLA_LOG) << path << query.toString();
    auto job = new TransferAPIJob(KIO::get(url(path, query), KIO::Reload, KIO::HideProgressInfo));
    return job;
}

APIJob *HTTPConnection::post(const QString &path, const QByteArray &data, const Query &query) const
{
    qCDebug(BUGZILLA_LOG) << path << query.toString();
    auto job = new TransferAPIJob(KIO::http_post(url(path, query), data, KIO::HideProgressInfo));
    return job;
}

APIJob *HTTPConnection::put(const QString &path, const QByteArray &data, const Query &query) const
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

QUrl HTTPConnection::url(const QString &appendix, Query query) const
{
    QUrl url(m_root);
    url.setPath(m_root.path() + appendix);

    if (!m_token.isEmpty()) {
        query.addQueryItem(QStringLiteral("token"), m_token);
    }

    // https://bugs.kde.org/show_bug.cgi?id=413920
    // Force encoding. Query by default wouldn't encode '+' and bugzilla doesn't like that...
    // For any query argument. Tested with username, password, and products (for bug search)
    // on bugzilla 5.0.6. As a result let's force full encoding on every argument.
    QUrlQuery escapedQuery;
    for (auto it = query.cbegin(); it != query.cend(); ++it) {
        escapedQuery.addQueryItem(it.key(), QString::fromUtf8(it.value().toUtf8().toPercentEncoding()));
    }

    url.setQuery(escapedQuery);
    return url;
}

} // namespace Bugzilla
