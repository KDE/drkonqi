/*******************************************************************
 * reportinterface.h
 * SPDX-FileCopyrightText: 2009, 2011 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef REPORTINTERFACE__H
#define REPORTINTERFACE__H

#include <QObject>
#include <QStringList>
#include <QTimer>

#include "sentrypostbox.h"

namespace Bugzilla
{
class NewBug;
} // namespace Bugzilla

class BugzillaManager;
class ProductMapping;

class ReportInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(BugzillaManager *bugzilla READ bugzillaManager CONSTANT)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString detailText MEMBER m_reportDetailText WRITE setDetailText NOTIFY detailTextChanged)
    Q_PROPERTY(bool userRememberCrashSitutation MEMBER m_userRememberCrashSituation NOTIFY awarenessChanged)
    Q_PROPERTY(Reproducible reproducible MEMBER m_reproducible NOTIFY awarenessChanged)
    Q_PROPERTY(bool provideActionsApplicationDesktop MEMBER m_provideActionsApplicationDesktop NOTIFY awarenessChanged)

    Q_PROPERTY(bool provideUnusualBehavior MEMBER m_provideUnusualBehavior NOTIFY provideUnusualBehaviorChanged)
    Q_PROPERTY(bool provideApplicationConfigurationDetails MEMBER m_provideApplicationConfigurationDetails NOTIFY awarenessChanged)
    Q_PROPERTY(QString backtrace READ backtrace WRITE setBacktrace NOTIFY backtraceChanged)

    Q_PROPERTY(bool isBugAwarenessPageDataUseful READ isBugAwarenessPageDataUseful NOTIFY awarenessChanged)

    Q_PROPERTY(uint attachToBugNumber READ attachToBugNumber WRITE setAttachToBugNumber NOTIFY attachToBugNumberChanged)
    Q_PROPERTY(uint duplicateId READ duplicateId WRITE setDuplicateId NOTIFY duplicateIdChanged)

    Q_PROPERTY(uint sentReport MEMBER m_sentReport NOTIFY done)
public:
    enum Reproducible {
        ReproducibleUnsure,
        ReproducibleNever,
        ReproducibleSometimes,
        ReproducibleEverytime,
    };
    Q_ENUM(Reproducible)

    enum class Backtrace {
        Complete,
        Reduced,
        Exclude,
    };
    Q_ENUM(Backtrace)

    enum class DrKonqiStamp {
        Include,
        Exclude,
    };
    Q_ENUM(DrKonqiStamp)

    static ReportInterface *self();

    Q_SIGNAL void awarenessChanged();

    Q_INVOKABLE void setBugAwarenessPageData(bool, ReportInterface::Reproducible, bool, bool, bool);
    bool isBugAwarenessPageDataUseful() const;

    Q_INVOKABLE int selectedOptionsRating() const;

    Q_INVOKABLE QStringList firstBacktraceFunctions() const;
    Q_INVOKABLE void setFirstBacktraceFunctions(const QStringList &functions);

    QString backtrace() const;
    void setBacktrace(const QString &backtrace);
    Q_SIGNAL void backtraceChanged();

    QString title() const;
    void setTitle(const QString &text);
    Q_SIGNAL void titleChanged();

    void setDetailText(const QString &text);
    Q_SIGNAL void detailTextChanged();

    Q_INVOKABLE void setPossibleDuplicates(const QStringList &duplicatesList);
    Q_INVOKABLE QString generateReportFullText(ReportInterface::DrKonqiStamp stamp, ReportInterface::Backtrace inlineBacktrace) const;

    Bugzilla::NewBug newBugReportTemplate() const;

    Q_INVOKABLE QStringList relatedBugzillaProducts() const;

    bool isWorthReporting() const;

    // Zero means creating a new bug report
    void setAttachToBugNumber(uint);
    uint attachToBugNumber() const;
    Q_SIGNAL void attachToBugNumberChanged();

    // Zero means there is no duplicate
    void setDuplicateId(uint);
    uint duplicateId() const;
    Q_SIGNAL void duplicateIdChanged();

    void setPossibleDuplicatesByQuery(const QStringList &);

    BugzillaManager *bugzillaManager() const;
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

    bool isCrashEventSendingEnabled() const;
    bool hasCrashEventSent() const;
    void setSendWhenReady(bool send);

public Q_SLOTS:
    void prepareCrashEvent();
    void prepareCrashComment();
    void sendBugReport();

private Q_SLOTS:
    void sendUsingDefaultProduct() const;
    // Attach backtrace to bug and use collected report as comment.
    void attachBacktraceWithReport();
    void attachSent(int);

Q_SIGNALS:
    void done();
    void sendReportError(const QString &);
    void provideUnusualBehaviorChanged();
    void crashEventSent();

private:
    explicit ReportInterface(QObject *parent = nullptr);
    // Attach backtrace to bug. Only used internally when the comment isn't
    // meant to be the full report.
    void attachBacktrace(const QString &comment);
    void sendToSentry();
    void maybeDone();
    void maybePickUpPostbox();

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

    QTimer m_sentryStartTimer;
    bool m_tryingSentry = false;
    SentryPostbox m_sentryPostbox;
    uint m_sentReport = 0;
    bool m_sendWhenReady = false;
};

#endif
