/*******************************************************************
 * parsebugbacktraces.cpp
 * SPDX-FileCopyrightText: 2011 Matthias Fuchs <mat69@gmx.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "parsebugbacktraces.h"

#include "parser/backtraceparser.h"

typedef QList<BacktraceLine>::const_iterator BacktraceConstIterator;

BacktraceConstIterator findCrashStackFrame(BacktraceConstIterator it, BacktraceConstIterator itEnd)
{
    BacktraceConstIterator result = itEnd;

    // find the beginning of the crash
    for (; it != itEnd; ++it) {
        if (it->type() == BacktraceLine::KCrash) {
            result = it;
            break;
        }
    }

    // find the beginning of the stack frame
    for (it = result; it != itEnd; ++it) {
        if (it->type() == BacktraceLine::StackFrame) {
            result = it;
            break;
        }
    }

    return result;
}

// TODO improve this stuff, it is just a HACK
ParseBugBacktraces::DuplicateRating rating(BacktraceConstIterator it, BacktraceConstIterator itEnd, BacktraceConstIterator it2, BacktraceConstIterator itEnd2)
{
    int matches = 0;
    int lines = 0;

    it = findCrashStackFrame(it, itEnd);
    it2 = findCrashStackFrame(it2, itEnd2);

    while (it != itEnd && it2 != itEnd2) {
        if (it->type() == BacktraceLine::StackFrame && it2->type() == BacktraceLine::StackFrame) {
            ++lines;
            if (it->frameNumber() == it2->frameNumber() && it->functionName() == it2->functionName()) {
                ++matches;
            }
            ++it;
            ++it2;
            continue;
        }

        // if iters do not point to emptylines or a stackframe increase them
        if (it->type() != BacktraceLine::StackFrame && it->type() != BacktraceLine::EmptyLine) {
            ++it;
            continue;
        }
        if (it2->type() != BacktraceLine::StackFrame && it2->type() != BacktraceLine::EmptyLine) {
            ++it2;
            continue;
        }

        // one bt is shorter than the other
        if (it->type() == BacktraceLine::StackFrame && it2->type() == BacktraceLine::EmptyLine) {
            ++lines;
            ++it;
            continue;
        }
        if (it2->type() == BacktraceLine::StackFrame && it->type() == BacktraceLine::EmptyLine) {
            ++lines;
            ++it2;
            continue;
        }

        if (it->type() == BacktraceLine::EmptyLine && it2->type() == BacktraceLine::EmptyLine) {
            // done
            break;
        }
    }

    if (!lines) {
        return ParseBugBacktraces::NoDuplicate;
    }

    const int rating = matches * 100 / lines;
    if (rating == 100) {
        return ParseBugBacktraces::PerfectDuplicate;
    } else if (rating >= 90) {
        return ParseBugBacktraces::MostLikelyDuplicate;
    } else if (rating >= 60) {
        return ParseBugBacktraces::MaybeDuplicate;
    } else {
        return ParseBugBacktraces::NoDuplicate;
    }
}

ParseBugBacktraces::ParseBugBacktraces(const QList<Bugzilla::Comment::Ptr> &comments, QObject *parent)
    : QObject(parent)
    , m_comments(comments)
{
    m_parser = BacktraceParser::newParser(QStringLiteral("gdb"), this);
    m_parser->connectToGenerator(this);
}

void ParseBugBacktraces::parse()
{
    for (const auto &comment : m_comments) {
        parse(comment->text());
    }
}

void ParseBugBacktraces::parse(const QString &comment)
{
    Q_EMIT starting();

    int start = 0;
    int end = -1;
    do {
        start = end + 1;
        end = comment.indexOf(QLatin1Char('\n'), start);
        Q_EMIT newLine(comment.mid(start, (end != -1 ? end - start + 1 : end)));
    } while (end != -1);

    // accepts anything as backtrace, the start of the backtrace is searched later anyway
    m_backtraces << m_parser->parsedBacktraceLines();
}

ParseBugBacktraces::DuplicateRating ParseBugBacktraces::findDuplicate(const QList<BacktraceLine> &backtrace)
{
    if (m_backtraces.isEmpty() || backtrace.isEmpty()) {
        return NoDuplicate;
    }

    DuplicateRating bestRating = NoDuplicate;
    DuplicateRating currentRating = NoDuplicate;

    QList<QList<BacktraceLine>>::const_iterator itBts;
    QList<QList<BacktraceLine>>::const_iterator itEndBts = m_backtraces.constEnd();
    for (itBts = m_backtraces.constBegin(); itBts != itEndBts; ++itBts) {
        currentRating = rating(backtrace.constBegin(), backtrace.constEnd(), itBts->constBegin(), itBts->constEnd());
        if (currentRating < bestRating) {
            bestRating = currentRating;
        }

        if (bestRating == PerfectDuplicate) {
            return bestRating;
        }
    }

    return bestRating;
}
