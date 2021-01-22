/*
    SPDX-FileCopyrightText: 2009-2010 George Kiagiadakis <kiagiadakis.george@gmail.com>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BACKTRACEPARSER_H
#define BACKTRACEPARSER_H

#include "backtraceline.h"
#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QStringList>
class BacktraceParserPrivate;

class BacktraceParser : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BacktraceParser)
public:
    enum Usefulness {
        InvalidUsefulness,
        Useless,
        ProbablyUseless,
        MayBeUseful,
        ReallyUseful,
    };
    Q_ENUM(Usefulness)

    static BacktraceParser *newParser(const QString &debuggerName, QObject *parent = nullptr);
    ~BacktraceParser() override;

    /*! Connects the parser to the backtrace generator.
     * Any QObject that defines the starting() and newLine(QString) signals will do.
     */
    void connectToGenerator(QObject *generator);

    /*! Returns the parsed backtrace. Any garbage that should not be shown to the user is removed. */
    virtual QString parsedBacktrace() const;

    /*! Same as parsedBacktrace(), but the backtrace here is returned as a list of
     * BacktraceLine objects, which provide extra information on each line.
     */
    virtual QList<BacktraceLine> parsedBacktraceLines() const;

    /*! Returns a simplified version of the backtrace. This backtrace:
     * \li Starts from the first useful function
     * \li Has maximum 5 lines
     * \li Replaces garbage with [...]
     */
    virtual QString simplifiedBacktrace() const;

    /*! Returns a value that indicates how much useful is the backtrace that we got */
    virtual Usefulness backtraceUsefulness() const;

    /*! Returns a short list of the first good functions that appear in the backtrace
     * (in the crashing thread). This is used for quering for duplicate reports.
     */
    virtual QStringList firstValidFunctions() const;

    /*! Returns a list of libraries/executables that are missing debug symbols. */
    virtual QSet<QString> librariesWithMissingDebugSymbols() const;

    /*! Check if the crash is because of the client aborting after a compositor crash.
     * https://bugs.kde.org/show_bug.cgi?id=431561
     */
    bool hasCompositorCrashed() const;

    QString informationLines() const;

private Q_SLOTS:
    void resetState();

protected Q_SLOTS:
    /*! Called every time there is a new line from the generator. Subclasses should parse
     * the line here and insert it in the m_linesList field of BacktraceParserPrivate.
     * If the line is useful for rating as well, it should also be inserted in the m_linesToRate
     * field, so that calculateRatingData() can use it.
     */
    virtual void newLine(const QString &lineStr) = 0;

protected:
    explicit BacktraceParser(QObject *parent = nullptr);

    /*! Subclasses should override to provide their own BacktraceParserPrivate instance */
    virtual BacktraceParserPrivate *constructPrivate() const;

    /*! This method should fill the m_usefulness, m_simplifiedBacktrace, m_firstValidFunctions
     * and m_librariesWithMissingDebugSymbols members of the BacktraceParserPrivate instance.
     * The default implementation uses the lines inserted in m_linesToRate and applies a
     * generic algorithm that should work for many debuggers.
     */
    virtual void calculateRatingData();

    BacktraceParserPrivate *d_ptr;
};

Q_DECLARE_METATYPE(BacktraceParser::Usefulness)

#endif // BACKTRACEPARSER_H
