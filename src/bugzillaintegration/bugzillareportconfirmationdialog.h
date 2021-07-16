/*******************************************************************
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#pragma once

#include "ui_assistantpage_bugzilla_duplicates_dialog_confirmation.h"

class BugzillaReportInformationDialog;

class BugzillaReportConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    BugzillaReportConfirmationDialog(int bugNumber, bool commonCrash, QString closedState, BugzillaReportInformationDialog *parent);
    ~BugzillaReportConfirmationDialog() override;

private Q_SLOTS:
    void proceedClicked();
    void checkProceed();

private:
    Ui::ConfirmationDialog ui;
    BugzillaReportInformationDialog *m_parent;
    bool m_showProceedQuestion;
    int m_bugNumber;
};
