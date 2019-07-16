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

#include <clients/bugfieldclient.h>

#include "jobdouble.h"

namespace Bugzilla
{

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
        if (path == "/field/bug/rep_platform" && query.toString() == "") {
            return new JobDouble { QFINDTESTDATA("data/field.rep_platform.json") };
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

class BugFieldTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Bugzilla::setConnection(m_doubleConnection);
    }

    void testGet()
    {
        Bugzilla::BugFieldClient client;
        KJob *job = client.getField("rep_platform");
        QVERIFY(job);
        job->start();
        auto field = client.getField(job);
        auto values = field->values();
        QCOMPARE(5, values.size());
        bool containsWindows = false;
        for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
            if ((*it)->name() == QLatin1String("MS Windows")) {
                containsWindows = true;
                break;
            }
        }
        QVERIFY(containsWindows);
        // Run again to drive up coverage for converter registration
        job = client.getField("rep_platform");
        QVERIFY(job);
        job->start();
        client.getField(job);
    }

private:
    Bugzilla::ConnectionDouble *m_doubleConnection = new Bugzilla::ConnectionDouble;
};

} // namespace Bugzilla

QTEST_MAIN(Bugzilla::BugFieldTest)

#include "bugfieldtest.moc"
