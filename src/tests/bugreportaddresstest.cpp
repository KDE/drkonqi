// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include <QTest>

#include <bugreportaddress.h>

using namespace Qt::StringLiterals;

class BugReportAddressTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testKDE()
    {
        QVERIFY(BugReportAddress(u"submit@bugs.kde.org"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"mailto:submit@bugs.kde.org"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"mailto:submit@bugs.kde.org?foo=bar"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"https://bugs.kde.org/enter_bug.cgi?product=plasma-systemmonitor"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"https://invent.kde.org/kde-linux/kde-linux"_s).isKdeBugzilla());
    }

    void testNotKDE()
    {
        QVERIFY(!BugReportAddress(u"submit@bugs.kde.org.uk"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"mailto:submit@bugs.kde.org.uk"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"mailto:submit@bugs.kde.org.uk?foo=bar"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"https://bugskde.org"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"https://bugs.kde.org.uk"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"https://submit@bugs.kde.org.uk"_s).isKdeBugzilla());
    }
};

QTEST_GUILESS_MAIN(BugReportAddressTest)

#include "bugreportaddresstest.moc"
