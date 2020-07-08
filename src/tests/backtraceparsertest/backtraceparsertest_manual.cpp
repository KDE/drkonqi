/*
    Copyright (C) 2010  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "fakebacktracegenerator.h"
#include "../../parser/backtraceparser.h"
#include <QFile>
#include <QSharedPointer>
#include <QTextStream>
#include <QMetaEnum>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <KAboutData>
#include <KLocalizedString>

#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("backtraceparsertest_manual"), i18n("backtraceparsertest_manual"), QStringLiteral("1.0"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("debugger"), i18n("The debugger name passed to the parser factory"), QStringLiteral("name"), QStringLiteral("gdb")));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("A file containing the backtrace."), QStringLiteral("[file]"));
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QString debugger = parser.value(QStringLiteral("debugger"));
    if(parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
        return 1;
    }
    QString file = parser.positionalArguments().first();

    if (!QFile::exists(file)) {
        std::cerr << "The specified file does not exist" << std::endl;
        return 1;
    }

    FakeBacktraceGenerator generator;
    QSharedPointer<BacktraceParser> btparser(BacktraceParser::newParser(debugger));
    btparser->connectToGenerator(&generator);
    generator.sendData(file);

    QMetaEnum metaUsefulness = BacktraceParser::staticMetaObject.enumerator(
                                BacktraceParser::staticMetaObject.indexOfEnumerator("Usefulness"));
    std::cout << "Usefulness: " << qPrintable(metaUsefulness.valueToKey(btparser->backtraceUsefulness())) << std::endl;
    std::cout << "First valid functions: " << qPrintable(btparser->firstValidFunctions().join(QLatin1Char(' '))) << std::endl;
    std::cout << "Simplified backtrace:\n" << qPrintable(btparser->simplifiedBacktrace()) << std::endl;
    const QStringList l = static_cast<QStringList>(btparser->librariesWithMissingDebugSymbols().values());
    std::cout << "Missing dbgsym libs: " << qPrintable(l.join(QLatin1Char(' '))) << std::endl;

    return 0;
}
