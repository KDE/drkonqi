/*******************************************************************
 * reportinterface.h
 * SPDX-FileCopyrightText: 2009, 2011 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef REPORTINTERFACE__H
#define REPORTINTERFACE__H

#include <QObject>
#include <QStringList>

namespace Bugzilla
{
class NewBug;
}

class BugzillaManager;
class ProductMapping;
class ApplicationDetailsExamples;

class ReportInterface : public QObject
{
    Q_OBJECT
public:
    enum Reproducible {
        ReproducibleUnsure,
        ReproducibleNever,
        ReproducibleSometimes,
        ReproducibleEverytime,
    };

    enum class Backtrace {
        Complete,
        Reduced,
        Exclude,
    };

    enum class DrKonqiStamp {
        Include,
        Exclude,
    };

    explicit ReportInterface(QObject *parent = nullptr);

    void setBugAwarenessPageData(bool, Reproducible, bool, bool, bool);
    bool isBugAwarenessPageDataUseful() const;

    int selectedOptionsRating() const;

    QStringList firstBacktraceFunctions() const;
    void setFirstBacktraceFunctions(const QStringList &functions);

    QString backtrace() const;
    void setBacktrace(const QString &backtrace);

    QString title() const;
    void setTitle(const QString &text);

    void setDetailText(const QString &text);
    void setPossibleDuplicates(const QStringList &duplicatesList);

    QString generateReportFullText(DrKonqiStamp stamp, Backtrace inlineBacktrace) const;

    Bugzilla::NewBug newBugReportTemplate() const;

    QStringList relatedBugzillaProducts() const;

    bool isWorthReporting() const;

    // Zero means creating a new bug report
    void setAttachToBugNumber(uint);
    uint attachToBugNumber() const;

    // Zero means there is no duplicate
    void setDuplicateId(uint);
    uint duplicateId() const;

    void setPossibleDuplicatesByQuery(const QStringList &);

    BugzillaManager *bugzillaManager() const;
    ApplicationDetailsExamples *appDetailsExamples() const;
    ProductMapping *productMapping() const;

    bool userCanProvideActionsAppDesktop() const
    {
        return m_provideActionsApplicationDesktop;
    }

    bool userCanProvideUnusualBehavior() const
    {
        return m_provideUnusualBehavior;
    }

    bool userCanProvideApplicationConfigDetails() const
    {
        return m_provideApplicationConfigurationDetails;
    }

public Q_SLOTS:
    void sendBugReport();

private Q_SLOTS:
    void sendUsingDefaultProduct() const;
    // Attach backtrace to bug and use collected report as comment.
    void attachBacktraceWithReport();
    void attachSent(int);

Q_SIGNALS:
    void reportSent(int);
    void sendReportError(const QString &);

private:
    // Attach backtrace to bug. Only used internally when the comment isn't
    // meant to be the full report.
    void attachBacktrace(const QString &comment);

    QString generateAttachmentComment() const;

    // Information the user can provide
    bool m_userRememberCrashSituation;
    Reproducible m_reproducible;
    bool m_provideActionsApplicationDesktop;
    bool m_provideUnusualBehavior;
    bool m_provideApplicationConfigurationDetails;

    QString m_backtrace;
    QStringList m_firstBacktraceFunctions;

    QString m_reportTitle;
    QString m_reportDetailText;
    QStringList m_possibleDuplicates;

    QStringList m_allPossibleDuplicatesByQuery;

    uint m_attachToBugNumber;
    uint m_duplicate;

    ProductMapping *m_productMapping = nullptr;
    BugzillaManager *m_bugzillaManager = nullptr;
    ApplicationDetailsExamples *m_appDetailsExamples = nullptr;
};

#endif
