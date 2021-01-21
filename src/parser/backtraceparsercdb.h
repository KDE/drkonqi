/*
    SPDX-FileCopyrightText: 2019 Patrick José Pereira <patrickelectric@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "backtraceparser.h"

class BacktraceParserCdb : public BacktraceParser
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BacktraceParser)

public:
    explicit BacktraceParserCdb(QObject *parent = nullptr);

protected Q_SLOTS:
    void newLine(const QString &lineStr) override;

protected:
    BacktraceParserPrivate *constructPrivate() const override;
};

class BacktraceLineCdb : public BacktraceLine
{
public:
    BacktraceLineCdb(const QString &line);
};
