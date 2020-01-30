/*
    Copyright 2020 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QTest>

#include "../parser/backtraceparsergdb.h"

class GdbBacktraceLineTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    // rating() is often times somewhat misleading because it is an exclusive state
    // but in practise a frame may have multiple issues. for purposes of rating all
    // issues are considered equal. it's not ideal though, a frame that is missing
    // multiple elements is objectively worse than a frame that is just missing one.

    void testComplete()
    {
        BacktraceLineGdb line(
                    "#7  0x00007f468b177bfa in KMime::DateFormatterPrivate::localized (t=t@entry=1579263464, shortFormat=shortFormat@entry=true, lang=...) at /usr/src/debug/kmime-19.12.1-lp151.150.1.x86_64/src/kmime_dateformatter.cpp:310\n"
                    );
        QCOMPARE(line.type(), BacktraceLine::StackFrame);
        QCOMPARE(line.frameNumber(), 7);
        QCOMPARE(line.functionName(), "KMime::DateFormatterPrivate::localized");
        QCOMPARE(line.fileName(), "/usr/src/debug/kmime-19.12.1-lp151.150.1.x86_64/src/kmime_dateformatter.cpp:310");
        QCOMPARE(line.libraryName(), "");
        QCOMPARE(line.rating(), BacktraceLine::Good);
    }

    void testPoorFile()
    {
        BacktraceLineGdb line("#41 0x00007f4684ae4e87 in g_main_context_dispatch () from /usr/lib64/libglib-2.0.so.0\n");
        QCOMPARE(line.type(), BacktraceLine::StackFrame);
        QCOMPARE(line.frameNumber(), 41);
        QCOMPARE(line.functionName(), "g_main_context_dispatch");
        QCOMPARE(line.fileName(), "");
        QCOMPARE(line.libraryName(), "/usr/lib64/libglib-2.0.so.0");
        QCOMPARE(line.rating(), BacktraceLine::MissingSourceFile);
    }

    void testNoFunctionPoorFile()
    {
        // Also uses 'at' keyword while referring to a library, this used to trip up
        // the parser and make it think there's a source file, when in reality there
        // is not.
        // Lacking a function name further tripped up the parsing because originally
        // couldn't deal with the function name being missing entirely.
        // As a result this line used to rate as 'Good' -.-
        // https://bugs.kde.org/show_bug.cgi?id=416923
        BacktraceLineGdb line("#13 0x00007fe6059971b1 in  () at /usr/lib/libglib-2.0.so.0\n");
        QCOMPARE(line.type(), BacktraceLine::StackFrame);
        QCOMPARE(line.frameNumber(), 13);
        QCOMPARE(line.functionName(), "");
        QCOMPARE(line.fileName(), "");
        QCOMPARE(line.libraryName(), "/usr/lib/libglib-2.0.so.0");
        QCOMPARE(line.rating(), BacktraceLine::MissingSourceFile);
    }

    void testOnlyFunctionNofile()
    {
        BacktraceLineGdb line("#20 0x0000557e978c1b7e in _start ()\n");
        QCOMPARE(line.type(), BacktraceLine::StackFrame);
        QCOMPARE(line.frameNumber(), 20);
        QCOMPARE(line.functionName(), "_start");
        QCOMPARE(line.fileName(), "");
        QCOMPARE(line.libraryName(), "");
        QCOMPARE(line.rating(), BacktraceLine::MissingLibrary);
    }

    void testInferiorMarker()
    {
        BacktraceLineGdb line("[Inferior 1 (process 72692) detached]\n");
        QCOMPARE(line.type(), BacktraceLine::Unknown);
        QCOMPARE(line.rating(), BacktraceLine::InvalidRating);
    }
};

QTEST_GUILESS_MAIN(GdbBacktraceLineTest)

#include "gdbbacktracelinetest.moc"
