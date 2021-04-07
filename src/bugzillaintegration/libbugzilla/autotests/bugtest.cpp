/*
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QTest>

#include <QDebug>

#include "../clients/bugclient.h"
#include "../clients/productclient.h"

#include "jobdouble.h"

namespace Bugzilla
{
static void compareNewBugHash(const QVariantHash &hash, bool *ok)
{
    *ok = false;
    QCOMPARE(hash["product"], "aproduct");
    QCOMPARE(hash["component"], "acomp");
    QCOMPARE(hash["summary"], "asummary");
    QCOMPARE(hash["version"], "aversion");
    QCOMPARE(hash["description"], "adescription");
    QCOMPARE(hash["op_sys"], "asys");
    QCOMPARE(hash["platform"], "aplatform");
    QCOMPARE(hash["priority"], "apriority");
    QCOMPARE(hash["severity"], "aseverity");
    QCOMPARE(hash["keywords"], QStringList({"aword", "anotherword"}));
    *ok = true;
}

static void compareUpdateBugHash(const QVariantHash &hash, bool *ok)
{
    *ok = false;
    QCOMPARE(hash["cc"].toHash()["add"].toStringList(), QStringList({"me@host.com"}));
    QCOMPARE(hash["cc"].toHash()["remove"].toStringList(), QStringList({"you@host.com"}));
    *ok = true;
}

class ConnectionDouble : public Connection
{
public:
    using Connection::Connection;

    virtual void setToken(const QString &) override
    {
        Q_UNREACHABLE();
    }

    virtual APIJob *get(const QString &path, const Query &query = Query()) const override
    {
        if (path == "/bug" && query.toString() == "product=dragonplayer") {
            return new JobDouble{QFINDTESTDATA("data/bugs.dragonplayer.json")};
        }
        if (path == "/bug" && query.toString() == "product=dragonplayer2") {
            return new JobDouble{QFINDTESTDATA("data/bugs.unresolved.json")};
        }
        Q_ASSERT_X(false, "get", qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *post(const QString &path, const QByteArray &data, const Query &query = Query()) const override
    {
        qDebug() << path << query.toString();
        if (path == "/bug" && query.isEmpty()) {
            QJsonParseError e;
            auto doc = QJsonDocument::fromJson(data, &e);
            Q_ASSERT(e.error == QJsonParseError::NoError);
            auto hash = doc.object().toVariantHash();
            bool ok;
            compareNewBugHash(hash, &ok);
            Q_ASSERT(ok);

            return new JobDouble{QFINDTESTDATA("data/bugs.new.json")};
        }
        Q_ASSERT_X(false, "post", qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }

    virtual APIJob *put(const QString &path, const QByteArray &data, const Query &query = Query()) const override
    {
        if (path == "/bug/54321" && query.isEmpty()) {
            QJsonParseError e;
            auto doc = QJsonDocument::fromJson(data, &e);
            Q_ASSERT(e.error == QJsonParseError::NoError);
            auto hash = doc.object().toVariantHash();
            bool ok;
            compareUpdateBugHash(hash, &ok);
            Q_ASSERT(ok);

            return new JobDouble{QFINDTESTDATA("data/bugs.update.json")};
        }
        Q_ASSERT_X(false, "put", qUtf8Printable(QStringLiteral("unmapped: %1; %2").arg(path, query.toString())));
        return nullptr;
    }
};

class BugTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void initTestCase()
    {
        Bugzilla::setConnection(m_doubleConnection);
    }

    void testSearch()
    {
        Bugzilla::BugSearch search;
        search.products = QStringList{"dragonplayer"};
        auto job = Bugzilla::BugClient().search(search);
        job->start();
        QList<Bug::Ptr> bugs = Bugzilla::BugClient().search(job);
        QCOMPARE(bugs.size(), 2);
        Bug::Ptr bug;
        for (auto it = bugs.begin(); it != bugs.end(); ++it) {
            if ((*it)->id() == 156514) {
                bug = *it;
            }
        }

        QCOMPARE(bug.isNull(), false);
        QCOMPARE(bug->id(), 156514);
        QCOMPARE(bug->product(), "dragonplayer");
        QCOMPARE(bug->component(), "general");
        QCOMPARE(bug->summary(), "Supported filetypes not shown in Play File.. Dialog");
        QCOMPARE(bug->version(), "unspecified");
        QCOMPARE(bug->op_sys(), "Linux");
        QCOMPARE(bug->priority(), "NOR");
        QCOMPARE(bug->severity(), "normal");
        QCOMPARE(bug->status(), Bug::Status::RESOLVED);
        QCOMPARE(bug->resolution(), Bug::Resolution::FIXED);
        QCOMPARE(bug->dupe_of(), -1);
        QCOMPARE(bug->is_open(), false);
        QCOMPARE(bug->customField("cf_versionfixedin"), "5.0");
    }

    void testSearchUnresolved()
    {
        Bugzilla::BugSearch search;
        search.products = QStringList{"dragonplayer2"};
        auto job = Bugzilla::BugClient().search(search);
        job->start();
        QList<Bug::Ptr> bugs = Bugzilla::BugClient().search(job);
        QCOMPARE(bugs.size(), 1);
        // resolution:"" maps to NONE
        QCOMPARE(bugs.at(0)->resolution(), Bug::Resolution::NONE);
        // None of the above should fail assertions or exception tests.
    }

    void testNewBug()
    {
        Bugzilla::NewBug bug;

        bug.product = "aproduct";
        bug.component = "acomp";
        bug.summary = "asummary";
        bug.version = "aversion";
        bug.description = "adescription";
        bug.op_sys = "asys";
        bug.platform = "aplatform";
        bug.priority = "apriority";
        bug.severity = "aseverity";
        bug.keywords = QStringList{"aword", "anotherword"};

        auto job = Bugzilla::BugClient().create(bug);
        job->start();
        qint64 id = Bugzilla::BugClient().create(job);
        QCOMPARE(id, 12345);
    }

    void testUpdateBug()
    {
        Bugzilla::BugUpdate bug;
        bug.cc->add << "me@host.com";
        bug.cc->remove << "you@host.com";

        auto job = Bugzilla::BugClient().update(54321, bug);
        job->start();
        qint64 id = Bugzilla::BugClient().update(job);
        QCOMPARE(id, 54321);
    }

private:
    Bugzilla::ConnectionDouble *m_doubleConnection = new Bugzilla::ConnectionDouble;
};

} // namespace Bugzilla

QTEST_MAIN(Bugzilla::BugTest)

#include "bugtest.moc"
