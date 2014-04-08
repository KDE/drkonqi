/*
    Copyright (C) 2010  Milian Wolff <mail@milianw.de>

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
#ifndef GDBHIGHLIGHTER_H
#define GDBHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include "parser/backtraceline.h"

class GdbHighlighter : public QSyntaxHighlighter
{
public:
    GdbHighlighter(QTextDocument* parent, const QList<BacktraceLine> & gdbLines);

protected:
    virtual void highlightBlock(const QString& text);

private:
    QMap<int, BacktraceLine> lines;
    QTextCharFormat crashFormat;
    QTextCharFormat nullptrFormat;
    QTextCharFormat assertFormat;
    QTextCharFormat threadFormat;
    QTextCharFormat urlFormat;
    QTextCharFormat funcFormat;
    QTextCharFormat otheridFormat;
    QTextCharFormat crapFormat;
};

#endif // GDBHIGHLIGHTER_H
