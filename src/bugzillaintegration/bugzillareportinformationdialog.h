/*******************************************************************
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#pragma once

#include "bugzillalib.h"

#include "ui_assistantpage_bugzilla_duplicates_dialog.h"

class BugzillaDuplicatesPage;

class BugzillaReportInformationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BugzillaReportInformationDialog(BugzillaDuplicatesPage *parent = nullptr);
    ~BugzillaReportInformationDialog() override;

    void showBugReport(int bugNumber, bool relatedButtonEnabled = true);
    void markAsDuplicate();
    void attachToBugReport();
    void cancelAssistant();

private Q_SLOTS:
    void bugFetchFinished(Bugzilla::Bug::Ptr bug, QObject *);
    void onCommentsFetched(QList<Bugzilla::Comment::Ptr> bugComments, QObject *jobOwner);
    void bugFetchError(QString, QObject *);
    void reloadReport();
    void relatedReportClicked();
    void toggleShowOwnBacktrace(bool);

Q_SIGNALS:
    void possibleDuplicateSelected(int);
    void attachToBugReportSelected(int);

private:
    Ui::AssistantPageBugzillaDuplicatesDialog ui;
    bool m_relatedButtonEnabled;
    BugzillaDuplicatesPage *m_parent;

    int m_bugNumber;
    QString m_closedStateString;
    int m_duplicatesCount;
    QPushButton *m_suggestButton;

    Bugzilla::Bug::Ptr m_bug = nullptr;

    Q_DISABLE_COPY_MOVE(BugzillaReportInformationDialog) // rule of 5
};
