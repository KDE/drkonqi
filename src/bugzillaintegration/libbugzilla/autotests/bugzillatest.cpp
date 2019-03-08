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

#include <QTest>

#include <QDebug>

//#include "../apijob.h"
#include "../bugzilla.h"

namespace Bugzilla {

class JobDouble : public APIJob
{
    Q_OBJECT
public:
    using APIJob::APIJob;

    JobDouble(QString fixture)
        : m_fixture(fixture)
    {
    }

    virtual QByteArray data() const override
    {
        Q_ASSERT(!m_fixture.isEmpty());
        QFile file(m_fixture);
        Q_ASSERT(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream in(&file);
        return in.readAll().toUtf8();
    }

    QString m_fixture;
};

class ConnectionDouble : public Connection
{
public:
    using Connection::Connection;

    virtual void setToken(const QString &) override
    {
        Q_UNREACHABLE();
    }

    virtual APIJob *get(const QString &path,
                        const QUrlQuery &query = QUrlQuery()) const override
    {
        if (path == "/version") {
            return new JobDouble { QFINDTESTDATA("data/bugzilla.version.json") };
        } else if (path == "/login" && query.toString() == "login=auser&password=apass&restrict_login=true") {
            return new JobDouble { QFINDTESTDATA("data/bugzilla.login.json") };
        }
        Q_ASSERT_X(false, "get",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *post(const QString &path,
                         const QByteArray &,
                         const QUrlQuery &query = QUrlQuery()) const override
    {
        Q_ASSERT_X(false, "post",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *put(const QString &path,
                        const QByteArray &,
                        const QUrlQuery &query = QUrlQuery()) const override
    {
        Q_ASSERT_X(false, "put",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }
};

} // namespace Bugzilla

class BugzillaTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Bugzilla::setConnection(m_doubleConnection);
    }

    void testVersion()
    {
        KJob *job = Bugzilla::version();
        job->start();
        QCOMPARE(Bugzilla::version(job), "5.0.0");
    }

    void testLogin()
    {
        KJob *job = Bugzilla::login("auser", "apass");
        job->start();
        auto details = Bugzilla::login(job);
        QCOMPARE(details.id, 52960);
        QCOMPARE(details.token, "52960-aaaaaaaaaa");
    }

private:
    Bugzilla::ConnectionDouble *m_doubleConnection = new Bugzilla::ConnectionDouble;
};

QTEST_GUILESS_MAIN(BugzillaTest)

#include "bugzillatest.moc"
