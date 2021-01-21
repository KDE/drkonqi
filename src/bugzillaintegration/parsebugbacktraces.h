/*******************************************************************
 * parsebugbacktraces.h
 * SPDX-FileCopyrightText: 2011 Matthias Fuchs <mat69@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef PARSE_BUG_BACKTRACES_H
#define PARSE_BUG_BACKTRACES_H

#include "bugzillalib.h"
#include "parser/backtraceline.h"

class BacktraceParser;

/**
 * Parses a Bugreport to find all the backtraces listed there
 * NOTE it assumes that the backtraces provided were created
 * by gdb
 */
class ParseBugBacktraces : QObject
{
    Q_OBJECT
public:
    explicit ParseBugBacktraces(const QList<Bugzilla::Comment::Ptr> &comments, QObject *parent = nullptr);

    void parse();

    enum DuplicateRating {
        PerfectDuplicate, // functionnames and stackframe numer match
        MostLikelyDuplicate, // functionnames and stackframe numer match >=90%
        MaybeDuplicate, // functionnames and stackframe numer match >=60%
        NoDuplicate, // functionnames and stackframe numer match <60%
    };

    DuplicateRating findDuplicate(const QList<BacktraceLine> &backtrace);

Q_SIGNALS:
    void starting();
    void newLine(const QString &line);

private:
    void parse(const QString &comment);

private:
    BacktraceParser *m_parser = nullptr;
    const QList<Bugzilla::Comment::Ptr> m_comments;
    QList<QList<BacktraceLine>> m_backtraces;
};

#endif
