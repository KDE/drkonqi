/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "fakebacktracegenerator.h"
#include <QFile>
#include <QTextStream>

void FakeBacktraceGenerator::sendData(const QString &filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);

    emit starting();
    while (!stream.atEnd()) {
        emit newLine(stream.readLine() + QLatin1Char('\n'));
    }
    emit newLine(QString());
}
