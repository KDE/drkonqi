/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "backtraceparsertest.h"
#include <QMetaEnum>
#include <QSharedPointer>

#define DATA_DIR QFINDTESTDATA("backtraceparsertest_data")

BacktraceParserTest::BacktraceParserTest(QObject *parent)
    : QObject(parent),
      m_settings(DATA_DIR + QLatin1Char('/') + QStringLiteral("data.ini"), QSettings::IniFormat),
      m_generator(new FakeBacktraceGenerator(this))
{
}

void BacktraceParserTest::fetchData(const QString & group)
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("result");
    QTest::addColumn<QString>("debugger");

    m_settings.beginGroup(group);
    QStringList keys = m_settings.allKeys();
    m_settings.endGroup();

    foreach(const QString & key, keys) {
        QTest::newRow(qPrintable(key))
            << QString(DATA_DIR + QLatin1Char('/') + key)
            << m_settings.value(group + QLatin1Char('/') + key).toString()
            << m_settings.value(QStringLiteral("debugger/") + key).toString();
    }
}

void BacktraceParserTest::btParserUsefulnessTest_data()
{
    fetchData(QStringLiteral("usefulness"));
}

void BacktraceParserTest::btParserUsefulnessTest()
{
    QFETCH(QString, filename);
    QFETCH(QString, result);
    QFETCH(QString, debugger);

    //parse
    QSharedPointer<BacktraceParser> parser(BacktraceParser::newParser(debugger));
    parser->connectToGenerator(m_generator);
    m_generator->sendData(filename);

    //convert usefulness to string
    QMetaEnum metaUsefulness = BacktraceParser::staticMetaObject.enumerator(
                                    BacktraceParser::staticMetaObject.indexOfEnumerator("Usefulness"));
    QString btUsefulness = QString::fromLatin1(metaUsefulness.valueToKey(parser->backtraceUsefulness()));

    //compare
    QEXPECT_FAIL("test_e", "Working on it", Continue);
    QCOMPARE(btUsefulness, result);
}

void BacktraceParserTest::btParserFunctionsTest_data()
{
    fetchData(QStringLiteral("firstValidFunctions"));
}

void BacktraceParserTest::btParserFunctionsTest()
{
    QFETCH(QString, filename);
    QFETCH(QString, result);
    QFETCH(QString, debugger);

    //parse
    QSharedPointer<BacktraceParser> parser(BacktraceParser::newParser(debugger));
    parser->connectToGenerator(m_generator);
    m_generator->sendData(filename);

    //compare
    QString functions = parser->firstValidFunctions().join(QStringLiteral("|"));
    QCOMPARE(functions, result);
}

void BacktraceParserTest::btParserBenchmark_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("debugger");

    m_settings.beginGroup(QStringLiteral("debugger"));
    QStringList keys = m_settings.allKeys();
    foreach(const QString & key, keys) {
        QTest::newRow(qPrintable(key))
            << QString(DATA_DIR + QLatin1Char('/') + key)
            << m_settings.value(key).toString();
    }
    m_settings.endGroup();
}

void BacktraceParserTest::btParserBenchmark()
{
    QFETCH(QString, filename);
    QFETCH(QString, debugger);

    QSharedPointer<BacktraceParser> parser(BacktraceParser::newParser(debugger));
    parser->connectToGenerator(m_generator);

    QBENCHMARK_ONCE {
        m_generator->sendData(filename);
    }
}

QTEST_GUILESS_MAIN(BacktraceParserTest)

