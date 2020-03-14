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
#include "gdbhighlighter.h"

#include <QRegularExpression>
#include <QTextDocument>

#include <KColorScheme>

GdbHighlighter::GdbHighlighter(QTextDocument* parent, const QList<BacktraceLine> & gdbLines)
    : QSyntaxHighlighter(parent)
{
    // setup line lookup
    int l = 0;
    foreach(const BacktraceLine& line, gdbLines) {
        lines.insert(l, line);
        l += line.toString().count(QLatin1Char('\n'));
    }

    // setup formats
    KColorScheme scheme(QPalette::Active);

    crashFormat.setForeground(scheme.foreground(KColorScheme::NegativeText));
    nullptrFormat.setForeground(scheme.foreground(KColorScheme::NegativeText));
    nullptrFormat.setFontWeight(QFont::Bold);
    assertFormat = nullptrFormat;
    threadFormat.setForeground(scheme.foreground(KColorScheme::NeutralText));
    urlFormat.setForeground(scheme.foreground(KColorScheme::LinkText));
    funcFormat.setForeground(scheme.foreground(KColorScheme::VisitedText));
    funcFormat.setFontWeight(QFont::Bold);
    otheridFormat.setForeground(scheme.foreground(KColorScheme::PositiveText));
    crapFormat.setForeground(scheme.foreground(KColorScheme::InactiveText));
}

void GdbHighlighter::highlightBlock(const QString& text)
{
    int cur = 0;
    int next;
    int diff;
    const static QRegularExpression hexptrPattern(QStringLiteral("0x[0-9a-f]+"));
    int lineNr = currentBlock().firstLineNumber();
    while ( cur < text.length() ) {
        next = text.indexOf(QLatin1Char('\n'), cur);
        if (next == -1) {
            next = text.length();
        }
        if (lineNr == 0) {
            // line that contains 'Application: ...'
            ++lineNr;
            cur = next;
            continue;
        }

        diff = next - cur;

        const QString lineStr = text.mid(cur, diff).append(QLatin1Char('\n'));
        // -1 since we skip the first line
        QMap< int, BacktraceLine >::iterator it = lines.lowerBound(lineNr - 1);
        Q_ASSERT(it != lines.end());
        // lowerbound would return the next higher item, even though we want the former one
        if (it.key() > lineNr - 1) {
            --it;
        }
        const BacktraceLine& line = it.value();

        if (line.type() == BacktraceLine::KCrash) {
            setFormat(cur, diff, crashFormat);
        } else if (line.type() == BacktraceLine::ThreadStart || line.type() == BacktraceLine::ThreadIndicator) {
            setFormat(cur, diff, threadFormat);
        } else if (line.type() == BacktraceLine::Crap) {
            setFormat(cur, diff, crapFormat);
        } else if (line.type() == BacktraceLine::StackFrame) {
            if (!line.fileName().isEmpty()) {
                int colonPos = line.fileName().lastIndexOf(QLatin1Char(':'));
                setFormat(lineStr.indexOf(line.fileName()), colonPos == -1 ? line.fileName().length() : colonPos, urlFormat);
            }
            if (!line.libraryName().isEmpty()) {
                setFormat(lineStr.indexOf(line.libraryName()), line.libraryName().length(), urlFormat);
            }
            if (!line.functionName().isEmpty()) {
                int idx = lineStr.indexOf(line.functionName());
                if (idx != -1) {
                    // highlight Id::Id::Id::Func
                    // Id should have otheridFormat, :: no format and Func funcFormat
                    int i = idx;
                    int from = idx;
                    while (i < idx + line.functionName().length()) {
                        if (lineStr.at(i) == QLatin1Char(':')) {
                            setFormat(from, i - from, otheridFormat);
                            // skip ::
                            i += 2;
                            from = i;
                            continue;
                        } else if (lineStr.at(i) == QLatin1Char('<') || lineStr.at(i) == QLatin1Char('>')) {
                            setFormat(from, i - from, otheridFormat);
                            ++i;
                            from = i;
                            continue;
                        }
                        ++i;
                    }
                    if (line.functionName() == QLatin1String("qFatal") || line.functionName() == QLatin1String("abort") || line.functionName() == QLatin1String("__assert_fail")
                        || line.functionName() == QLatin1String("*__GI___assert_fail") || line.functionName() == QLatin1String("*__GI_abort")) {
                        setFormat(from, i - from, assertFormat);
                    } else {
                        setFormat(from, i - from, funcFormat);
                    }
                }
            }
            // highlight hexadecimal ptrs
            QRegularExpressionMatchIterator iter = hexptrPattern.globalMatch(lineStr);
            while (iter.hasNext()) {
                const QRegularExpressionMatch match = iter.next();
                if (match.captured(0) == QLatin1String("0x0")) {
                    setFormat(match.capturedStart(0), match.capturedLength(0), nullptrFormat);
                }
            }
        }

        cur = next;
        ++lineNr;
    }
}
