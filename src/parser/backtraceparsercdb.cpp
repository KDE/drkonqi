/*
    Copyright (C) 2019 Patrick José Pereira <patrickelectric@gmail.com>

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