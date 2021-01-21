/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BACKTRACEPARSERTEST_H
#define BACKTRACEPARSERTEST_H

#include "../../parser/backtraceparser.h"
#include "fakebacktracegenerator.h"
#include <QSettings>
#include <QTest>

class BacktraceParserTest : public QObject
{
    Q_OBJECT
public:
    BacktraceParserTest(QObject *parent = nullptr);

private Q_SLOTS:
    void btParserUsefulnessTest_data();
    void btParserUsefulnessTest();
    void btParserFunctionsTest_data();
    void btParserFunctionsTest();
    void btParserBenchmark_data();
    void btParserBenchmark();

private:
    void fetchData(const QString &group);

    QSettings m_settings;
    FakeBacktraceGenerator *m_generator = nullptr;
};

#endif
