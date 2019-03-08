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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <QException>

class QJsonDocument;
class QJsonObject;

namespace KIO {
class TransferJob;
}

namespace Bugzilla {

class APIJob;

/**
 * Root class for exceptions. Simply a QException which has the what backed
 * by a QString.
 */
class Exception : public QException
{
public:
    using QException::QException;
    virtual ~Exception();

    virtual QString whatString() const = 0;
    virtual const char *what() const noexcept override;

private:
    char *m_what = nullptr;
};

/**
 * Generic runtime exception.
 */
class RuntimeException : public Exception
{
public:
    RuntimeException(const QString &reason);
    virtual RuntimeException *clone() const override { return new RuntimeException(*this); }
    virtual QString whatString() const override;

private:
    QString m_reason;
};

/**
 * Translates an API error into an excpetion for easy handling.
 * Specifically when the API sends an error object in the body attempting to
 * access the JSON blob through one of the convenience accessors
 * (e.g. job.object()) will instead raise an exception.
 */
class APIException : public Exception
{
public:
    APIException(const QJsonDocument &document);
    APIException(const QJsonObject &object);
    APIException(const APIException &other);

    virtual void raise() const override { throw *this; }
    virtual APIException *clone() const override { return new APIException(*this); }
    virtual QString whatString() const override;

    bool isError() const { return m_isError; }

    static void maybeThrow(const QJsonDocument &document);

private:
    bool m_isError = false;
    QString m_message;
    int m_code = -1;
};

/**
 * Translates an KJob/APIJob error into an excpetion for easy handling.
 */
class ProtocolException : public Exception
{
public:
    ProtocolException(const APIJob *job);
    ProtocolException(const ProtocolException &other);

    virtual void raise() const override { throw *this; }
    virtual ProtocolException *clone() const override { return new ProtocolException(*this); }
    virtual QString whatString() const override;

    static void maybeThrow(const APIJob *job);

private:
    const APIJob *m_job = nullptr;
};

} // namespace Bugzilla

#endif // EXCEPTIONS_H
