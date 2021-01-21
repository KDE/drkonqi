/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * SPDX-FileCopyrightText: 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *****************************************************************/

#ifndef BACKTRACEGENERATOR_H
#define BACKTRACEGENERATOR_H

#include <QProcess>
#include <QTemporaryFile>

#include "debugger.h"

class KProcess;
class BacktraceParser;

class BacktraceGenerator : public QObject
{
    Q_OBJECT

public:
    enum State {
        NotLoaded,
        Loading,
        Loaded,
        Failed,
        FailedToStart,
    };

    BacktraceGenerator(const Debugger &debugger, QObject *parent);
    ~BacktraceGenerator() override;

    State state() const
    {
        return m_state;
    }

    BacktraceParser *parser() const
    {
        return m_parser;
    }

    QString backtrace() const
    {
        return m_parsedBacktrace;
    }

    const Debugger debugger() const
    {
        return m_debugger;
    }

public Q_SLOTS:
    bool start();

Q_SIGNALS:
    void starting();
    void newLine(const QString &str); // emitted for every line
    void someError();
    void failedToStart();
    void done();

private Q_SLOTS:
    void slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus);
    void slotReadInput();

private:
    const Debugger m_debugger;
    KProcess *m_proc = nullptr;
    QTemporaryFile *m_temp = nullptr;
    QByteArray m_output;
    State m_state;
    BacktraceParser *m_parser = nullptr;
    QString m_parsedBacktrace;

#ifdef BACKTRACE_PARSER_DEBUG
    BacktraceParser *m_debugParser = nullptr;
#endif
};

#endif
