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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QUrl>
#include <QUrlQuery>

#include "exceptions.h"
#include "apijob.h"

namespace Bugzilla {

class APIJob;

/**
 * Base Connection. Has CRUD-y methods that need implementing in a specific
 * connection variant.
 */
class Connection : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    virtual void setToken(const QString &authToken) = 0;

    virtual APIJob *get(const QString &path, const QUrlQuery &query = QUrlQuery()) const = 0;
    virtual APIJob *post(const QString &path, const QByteArray &data, const QUrlQuery &query = QUrlQuery()) const = 0;
    virtual APIJob *put(const QString &path, const QByteArray &data, const QUrlQuery &query = QUrlQuery()) const = 0;
};

/**
 * HTTP Connection.
 */
class HTTPConnection : public Connection
{
    Q_OBJECT
public:
    explicit HTTPConnection(const QUrl &root = QUrl(QStringLiteral("http://bugstest.kde.org/rest")),
                            QObject *parent = nullptr);
    ~HTTPConnection();

    virtual void setToken(const QString &authToken) override;

    virtual APIJob *get(const QString &path, const QUrlQuery &query = QUrlQuery()) const override;
    virtual APIJob *post(const QString &path, const QByteArray &data, const QUrlQuery &query = QUrlQuery()) const override;
    virtual APIJob *put(const QString &path, const QByteArray &data, const QUrlQuery &query = QUrlQuery()) const override;

    QUrl root() const;

private:
    QUrl url(const QString &appendix, QUrlQuery query) const;

    QUrl m_root;
    QString m_token;
};

/**
 * @return the "default" global connection instance. This is the instance used
 *   by all clients unless another one is manually set on the client.
 */
Connection &connection();

/**
 * Changes the "default" global connection. This generally shouldn't be used
 * outside tests, where it is used to inject connection doubles.
 */
void setConnection(Connection *newConnection);

} // namespace Bugzilla

#endif // CONNECTION_H
