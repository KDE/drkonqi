/*
    SPDX-FileCopyrightText: 2010 George Kiagiadakis <kiagiadakis.george@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BACKTRACEPARSERKDBGWIN_H
#define BACKTRACEPARSERKDBGWIN_H

#include "backtraceparser.h"

class BacktraceParserKdbgwin : public BacktraceParser
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BacktraceParser)
public:
    explicit BacktraceParserKdbgwin(QObject *parent = nullptr);

protected Q_SLOTS:
    void newLine(const QString &lineStr) override;
};

#endif // BACKTRACEPARSERKDBGWIN_H
