/*
    SPDX-FileCopyrightText: 2009-2010 George Kiagiadakis <kiagiadakis.george@gmail.com>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BACKTRACEPARSER_P_H
#define BACKTRACEPARSER_P_H

#include "backtraceparser.h"

class BacktraceParserPrivate
{
public:
    BacktraceParserPrivate()
        : m_usefulness(BacktraceParser::InvalidUsefulness)
    {
    }
    ~BacktraceParserPrivate()
    {
    }

    QStringList m_infoLines;
    QList<BacktraceLine> m_linesList;
    QList<BacktraceLine> m_linesToRate;
    QStringList m_firstUsefulFunctions;
    QString m_simplifiedBacktrace;
    QSet<QString> m_librariesWithMissingDebugSymbols;
    BacktraceParser::Usefulness m_usefulness;
    bool m_compositorCrashed = false;
};

#endif // BACKTRACEPARSER_P_H
