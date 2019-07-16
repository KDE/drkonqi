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

#include <QDebug>
#include <QTest>

#include <clients/productclient.h>

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
        Q_UNUSED(path);
        Q_UNUSED(query);
        if (path == "/product/dragonplayer") {
            return new JobDouble { QFINDTESTDATA("data/product.dragonplayer.json") };
        }
        Q_ASSERT_X(false, "get",
                   qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *post(const QString &path,
                         const QByteArray &,
                         const QUrlQuery &query = QUrlQuery()) const override
    {
        qDebug() << path << query.toString();
        Q_UNREACHABLE();
        return nullptr;
    }

    virtual APIJob *put(const QString &,
                        const QByteArray &,
                        const QUrlQuery & = QUrlQuery()) const override
    {
        Q_UNREACHABLE();
    }
};

class ProductTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Bugzilla::setConnection(m_doubleConnection);
    }

    void testProduct()
    {
        KJob *job = Bugzilla::ProductClient().get("dragonplayer");
        Q_ASSERT(job);
        job->start();
        Product::Ptr product = Bugzilla::ProductClient().get(job);

        QCOMPARE(product->isActive(), true);
        QCOMPARE(product->componentNames(), QStringList({"general"}));
        QCOMPARE(product->allVersions(),
                 QStringList({"2.0", "2.0-beta1", "2.0-git", "2.0.x", "17.04",
                              "17.08", "17.12", "18.04", "18.08", "18.12",
                              "SVN", "unspecified"}));

        QCOMPARE(product->versions().size(), 12);
        auto version = product->versions()[0];
        QCOMPARE(version->id(), 4408);
        QCOMPARE(version->name(), "2.0");
        QCOMPARE(version->isActive(), false);

        QCOMPARE(product->components().size(), 1);
        auto component = product->components()[0];
        QCOMPARE(component->id(), 1200);
        QCOMPARE(component->name(), "general");
    }

private:
    Bugzilla::ConnectionDouble *m_doubleConnection = new Bugzilla::ConnectionDouble;
};

} // namespace Bugzilla

QTEST_MAIN(Bugzilla::ProductTest)

#include "producttest.moc"
