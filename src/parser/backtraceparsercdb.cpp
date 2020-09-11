/*
    SPDX-FileCopyrightText: 2019 Patrick José Pereira <patrickelectric@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "backtraceparsercdb.h"
#include "backtraceparser_p.h"

BacktraceParserCdb::BacktraceParserCdb(QObject *parent)
    : BacktraceParser(parent)
{
}

BacktraceParserPrivate *BacktraceParserCdb::constructPrivate() const
{
    BacktraceParserPrivate *d = BacktraceParser::constructPrivate();
    d->m_usefulness = MayBeUseful;
    return d;
}

void BacktraceParserCdb::newLine(const QString &lineStr)
{
    d_ptr->m_linesList.append(BacktraceLineCdb(lineStr));
}

BacktraceLineCdb::BacktraceLineCdb(const QString &line)
    : BacktraceLine()
{
    d->m_line = line;
    // We should do the faith jump to believe that cdb will provides useful information
    d->m_rating = Good;
}