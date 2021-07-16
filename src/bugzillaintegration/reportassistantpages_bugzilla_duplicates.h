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

#include <KGuiItem>

#include "bugzillalib.h"
#include "duplicatefinderjob.h"
#include "ui_assistantpage_bugzilla_duplicates.h"

class BugzillaReportInformationDialog;

/** Searching for duplicates and showing report information page**/
class BugzillaDuplicatesPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaDuplicatesPage(ReportAssistantDialog *);

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

#endif
