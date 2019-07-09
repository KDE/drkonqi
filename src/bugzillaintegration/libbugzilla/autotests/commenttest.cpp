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


#include "../clients/commentclient.h"

namespace Bugzilla
{

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
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            return {};
        }
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
        if (path == "/bug/407363/comment" && query.toString().isEmpty()) {
            return new JobDouble { QFINDTESTDATA("data/comments.json") };
        }
        if (path == "/bug/1/comment" && query.toString().isEmpty()) {
            return new JobDouble { QFINDTESTDATA("data/error.nobug.invalid.json") };
        }
        Q_ASSERT_X(false, "get",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *post(const QString &path,
                         const QByteArray &,
                         const QUrlQuery &query = QUrlQuery()) const override
    {
        Q_UNUSED(path);
        Q_UNUSED(query);
        Q_ASSERT_X(false, "post",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *put(const QString &path,
                        const QByteArray &,
                        const QUrlQuery &query = QUrlQuery()) const override
    {
        Q_UNUSED(path);
        Q_UNUSED(query);
        Q_ASSERT_X(false, "put",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }
};

class CommentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void initTestCase()
    {
        Bugzilla::setConnection(m_doubleConnection);
    }

    void testSearch()
    {
        Bugzilla::CommentClient c;
        auto job = c.getFromBug(407363);
        job->start();
        auto comments = c.getFromBug(job);
        QCOMPARE(comments.size(), 3);

        Comment::Ptr uno = comments[0];
        QCOMPARE(uno->bug_id(), 407363);
        QCOMPARE(uno->text(), "uno");

        Comment::Ptr tre = comments[2];
        QCOMPARE(tre->bug_id(), 407363);
        QCOMPARE(tre->text(), "tre");
    }

    void testSearchNoBugInvalid()
    {
        // Our bugzilla has a bug where errors do not have error:true!
        // Make sure we correctly handle objects that are errors but do not
        // necessarily have error:true.
        // This is particularly relevant for comments because we make
        // expectations about bugs being valid/invalid/throwing.
        Bugzilla::CommentClient c;
        auto job = c.getFromBug(1);
        job->start();
        QVERIFY_EXCEPTION_THROWN(c.getFromBug(job), Bugzilla::APIException);
    }

private:
    Bugzilla::ConnectionDouble *m_doubleConnection = new Bugzilla::ConnectionDouble;
};

} // namespace Bugzilla

QTEST_MAIN(Bugzilla::CommentTest)

#include "commenttest.moc"
