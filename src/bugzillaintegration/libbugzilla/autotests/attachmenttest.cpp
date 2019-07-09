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


#include "../clients/attachmentclient.h"

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
        Q_UNUSED(path)
        Q_UNUSED(query)
        Q_ASSERT_X(false, "get",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *post(const QString &path,
                         const QByteArray &data,
                         const QUrlQuery &query = QUrlQuery()) const override
    {
        Q_UNUSED(query);
        if (path == "/bug/1/attachment") {
            QJsonParseError error;
            QJsonDocument::fromJson(data, &error);
            Q_ASSERT(error.error == QJsonParseError::NoError);
            return new JobDouble { QFINDTESTDATA("data/attachment.new.json") };
        }
        Q_ASSERT_X(false, "post",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *put(const QString &path,
                        const QByteArray &,
                        const QUrlQuery &query = QUrlQuery()) const override
    {
        Q_UNUSED(path)
        Q_UNUSED(query)
        Q_ASSERT_X(false, "put",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }
};

class AttachmentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void initTestCase()
    {
        Bugzilla::setConnection(m_doubleConnection);
    }

    void testCreate()
    {
        Bugzilla::AttachmentClient c;
        NewAttachment attachment;
        attachment.ids = { 1 };
        attachment.data = "123";
        attachment.file_name = "filename123";
        attachment.summary = "summary123";
//        attachment.content_type = "123";
        attachment.comment = "comment123";
        attachment.is_patch = true;
        attachment.is_private = false;
        auto job = c.createAttachment(1, attachment);
        job->start();
        QList<int> ids = c.createAttachment(job);
        QCOMPARE(ids, QList<int>({1234}));
    }

private:
    Bugzilla::ConnectionDouble *m_doubleConnection = new Bugzilla::ConnectionDouble;
};

} // namespace Bugzilla

QTEST_MAIN(Bugzilla::AttachmentTest)

#include "attachmenttest.moc"
