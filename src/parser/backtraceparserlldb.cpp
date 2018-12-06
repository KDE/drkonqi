/*
    Copyright (C) 2014 René J.V. Bertin <rjvbertin@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "backtraceparserlldb.h"
#include "backtraceparser_p.h"

//BEGIN BacktraceParserLldb

class BacktraceLineLldb : public BacktraceLine
{
public:
    BacktraceLineLldb(const QString & line);
};

BacktraceLineLldb::BacktraceLineLldb(const QString & line)
    : BacktraceLine()
{
    d->m_line = line;
    // For now we'll have faith that lldb provides useful information, and that it would
    // be unwarranted to give it a rating of "MissingEverything".
    d->m_rating = Good;
}

//END BacktraceLineLldb

//BEGIN BacktraceParserLldb

BacktraceParserLldb::BacktraceParserLldb(QObject *parent) : BacktraceParser(parent) {}

BacktraceParserPrivate *BacktraceParserLldb::constructPrivate() const
{
    BacktraceParserPrivate *d = BacktraceParser::constructPrivate();
    d->m_usefulness = MayBeUseful;
    return d;
}

void BacktraceParserLldb::newLine(const QString & lineStr)
{
    d_ptr->m_linesList.append(BacktraceLineLldb(lineStr));
}


//END BacktraceParserLldb
