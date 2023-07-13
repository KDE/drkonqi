/*
    SPDX-FileCopyrightText: 2010 George Kiagiadakis <kiagiadakis.george@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "backtraceparserkdbgwin.h"
#include "backtraceparser_p.h"
#include "drkonqi_parser_debug.h"

#include <QRegularExpression>

// BEGIN BacktraceLineKdbgwin

class BacktraceLineKdbgwin : public BacktraceLine
{
public:
    BacktraceLineKdbgwin(const QString &line);

private:
    void parse();
    void rate();
};

BacktraceLineKdbgwin::BacktraceLineKdbgwin(const QString &line)
    : BacktraceLine()
{
    d->m_line = line;
    parse();
    if (d->m_type == StackFrame) {
        rate();
    }
}

void BacktraceLineKdbgwin::parse()
{
    if (d->m_line == QLatin1Char('\n')) {
        d->m_type = EmptyLine;
        return;
    } else if (d->m_line == QLatin1String("[KCrash Handler]\n")) {
        d->m_type = KCrash;
        return;
    } else if (d->m_line.startsWith(QLatin1String("Loaded"))) {
        d->m_type = Crap; // FIXME that's not exactly crap
        return;
    }

    static const QRegularExpression re(
        QRegularExpression::anchoredPattern(QStringLiteral("([^!]+)!" // match the module name, followed by !
                                                           "([^\\(]+)\\(\\) " // match the function name, followed by ()
                                                           "\\[([^@]+)@ [\\-\\d]+\\] " // [filename @ line]
                                                           "at 0x.*"))); // at 0xdeadbeef

    const QRegularExpressionMatch match = re.match(d->m_line);
    if (match.hasMatch()) {
        d->m_type = StackFrame;
        d->m_library = match.captured(1);
        d->m_functionName = match.captured(2);
        d->m_file = match.captured(3).trimmed();

        qCDebug(DRKONQI_PARSER_LOG) << d->m_functionName << d->m_file << d->m_library;
        return;
    }

    qCDebug(DRKONQI_PARSER_LOG) << "line" << d->m_line << "did not match";
}

void BacktraceLineKdbgwin::rate()
{
    LineRating r;

    // for explanations, see the LineRating enum definition
    if (fileName() != QLatin1String("[unknown]")) {
        r = Good;
    } else if (libraryName() != QLatin1String("[unknown]")) {
        if (functionName() == QLatin1String("[unknown]")) {
            r = MissingFunction;
        } else {
            r = MissingSourceFile;
        }
    } else {
        if (functionName() == QLatin1String("[unknown]")) {
            r = MissingEverything;
        } else {
            r = MissingLibrary;
        }
    }

    d->m_rating = r;
}

// END BacktraceLineKdbgwin

// BEGIN BacktraceParserKdbgwin

BacktraceParserKdbgwin::BacktraceParserKdbgwin(QObject *parent)
    : BacktraceParser(parent)
{
}

void BacktraceParserKdbgwin::newLine(const QString &lineStr)
{
    Q_D(BacktraceParser);

    BacktraceLineKdbgwin line(lineStr);
    switch (line.type()) {
    case BacktraceLine::Crap:
        break; // we don't want crap in the backtrace ;)
    case BacktraceLine::StackFrame:
        d->m_linesToRate.append(line);
        Q_FALLTHROUGH();
    default:
        d->m_linesList.append(line);
    }
}

// END BacktraceParserKdbgwin

#include "moc_backtraceparserkdbgwin.cpp"
