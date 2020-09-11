/*******************************************************************
* reportassistantpages_bugzilla_duplicates.h
* SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
* SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-or-later
*
******************************************************************/

#ifndef REPORTASSISTANTPAGES__BUGZILLA__DUPLICATES_H
#define REPORTASSISTANTPAGES__BUGZILLA__DUPLICATES_H

#include "reportassistantpage.h"

#include "duplicatefinderjob.h"
#include "bugzillalib.h"

#include "ui_assistantpage_bugzilla_duplicates.h"
#include "ui_assistantpage_bugzilla_duplicates_dialog.h"
#include "ui_assistantpage_bugzilla_duplicates_dialog_confirmation.h"
#include <QDate>
#include <QDialog>
#include <KGuiItem>

class QDate;
class QTreeWidgetItem;

class KGuiItem;

class BugzillaReportInformationDialog;

/** Searching for duplicates and showing report information page**/
class BugzillaDuplicatesPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaDuplicatesPage(ReportAssistantDialog *);
    ~BugzillaDuplicatesPage() override;

    void aboutToShow() override;
    void aboutToHide() override;

    bool isComplete() override;
    bool showNextPage() override;

private Q_SLOTS:
    /* Search related methods */
    void searchMore();
    void stopCurrentSearch();

    void markAsSearching(bool);

    bool canSearchMore();

    void searchFinished(const QList<Bugzilla::Bug::Ptr> &);
    void searchError(QString);
    void analyzedDuplicates(KJob *job);

    /* Duplicates list related methods */
    void openSelectedReport();
    void itemClicked(QTreeWidgetItem *, int);
    void itemClicked(QListWidgetItem *);
    void showReportInformationDialog(int, bool relatedButtonEnabled = true);
    void itemSelectionChanged();

    /* Selected duplicates list related methods */
    void addPossibleDuplicateNumber(int);
    void removeSelectedDuplicate();

    void showDuplicatesPanel(bool);

    void possibleDuplicateSelectionChanged();

    /* Attach to bug related methods */
    void attachToBugReport(int);
    void cancelAttachToBugReport();
    void informationClicked(const QString &activatedLink);

private:
    bool m_searching = false;
    bool m_foundDuplicate = false;

    Ui::AssistantPageBugzillaDuplicates ui;

    KGuiItem m_searchMoreGuiItem;
    KGuiItem m_retrySearchGuiItem;
    DuplicateFinderJob::Result m_result;

    int m_offset = -1;
    bool m_atEnd = false;
};

/** Internal bug-info dialog **/
class BugzillaReportInformationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BugzillaReportInformationDialog(BugzillaDuplicatesPage*parent=nullptr);
    ~BugzillaReportInformationDialog() override;

    void showBugReport(int bugNumber, bool relatedButtonEnabled = true);

    void markAsDuplicate();
    void attachToBugReport();
    void cancelAssistant();

private Q_SLOTS:
    void bugFetchFinished(Bugzilla::Bug::Ptr bug, QObject *);
    void onCommentsFetched(QList<Bugzilla::Comment::Ptr> bugComments,
                           QObject *jobOwner);

    void bugFetchError(QString, QObject *);

    void reloadReport();

    void relatedReportClicked();

    void toggleShowOwnBacktrace(bool);

Q_SIGNALS:
    void possibleDuplicateSelected(int);
    void attachToBugReportSelected(int);

private:
    Ui::AssistantPageBugzillaDuplicatesDialog   ui;
    bool                                        m_relatedButtonEnabled;
    BugzillaDuplicatesPage *                    m_parent;

    int                                         m_bugNumber;
    QString                                     m_closedStateString;
    int                                         m_duplicatesCount;
    QPushButton*                                m_suggestButton;

    Bugzilla::Bug::Ptr m_bug = nullptr;
};

class BugzillaReportConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    BugzillaReportConfirmationDialog(int bugNumber, bool commonCrash, QString closedState,
                                     BugzillaReportInformationDialog * parent);
    ~BugzillaReportConfirmationDialog() override;

private Q_SLOTS:
    void proceedClicked();

    void checkProceed();

private:
    Ui::ConfirmationDialog              ui;

    BugzillaReportInformationDialog *   m_parent;

    bool                                m_showProceedQuestion;

    int                                 m_bugNumber;
};
#endif
