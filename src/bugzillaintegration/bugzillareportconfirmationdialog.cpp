/*******************************************************************
 * reportassistantpages_bugzilla_duplicates.cpp
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "bugzillareportconfirmationdialog.h"

#include <QDebug>

#include <KLocalizedString>

#include "bugzillareportinformationdialog.h"

BugzillaReportConfirmationDialog::BugzillaReportConfirmationDialog(int bugNumber,
                                                                   bool commonCrash,
                                                                   QString closedState,
                                                                   BugzillaReportInformationDialog *parent)
    : QDialog(parent)
    , m_parent(parent)
    , m_showProceedQuestion(false)
    , m_bugNumber(bugNumber)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setModal(true);

    ui.setupUi(this);

    // Setup dialog
    setWindowTitle(i18nc("@title:window", "Related Bug Report"));

    // Setup buttons
    ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(i18nc("@action:button", "Cancel (Go back to the report)"));
    ui.buttonBox->button(QDialogButtonBox::Ok)
        ->setText(i18nc("@action:button continue with the selected option "
                        "and close the dialog",
                        "Continue"));
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(this, &BugzillaReportConfirmationDialog::accepted, this, &BugzillaReportConfirmationDialog::proceedClicked);
    connect(this, &BugzillaReportConfirmationDialog::rejected, this, &BugzillaReportConfirmationDialog::hide);

    // Set introduction text
    ui.introLabel->setText(i18n("You are going to mark your crash as related to bug %1", QString::number(m_bugNumber)));

    if (commonCrash) { // Common ("massive") crash
        m_showProceedQuestion = true;
        ui.commonCrashIcon->setPixmap(QIcon::fromTheme(QStringLiteral("edit-bomb")).pixmap(22, 22));
    } else {
        ui.commonCrashLabel->setVisible(false);
        ui.commonCrashIcon->setVisible(false);
    }

    if (!closedState.isEmpty()) { // Bug report closed
        ui.closedReportLabel->setText(i18nc("@info",
                                            "The report is closed because %1. "
                                            "<i>If the crash is the same, adding further information will be useless "
                                            "and will consume developers' time.</i>",
                                            closedState));
        ui.closedReportIcon->setPixmap(QIcon::fromTheme(QStringLiteral("document-close")).pixmap(22, 22));
        m_showProceedQuestion = true;
    } else {
        ui.closedReportLabel->setVisible(false);
        ui.closedReportIcon->setVisible(false);
    }

    // Disable all the radio buttons
    ui.proceedRadioYes->setChecked(false);
    ui.proceedRadioNo->setChecked(false);
    ui.markAsDuplicateCheck->setChecked(false);
    ui.attachToBugReportCheck->setChecked(false);

    connect(ui.buttonGroupProceed, SIGNAL(buttonClicked(int)), this, SLOT(checkProceed()));
    connect(ui.buttonGroupProceedQuestion, SIGNAL(buttonClicked(int)), this, SLOT(checkProceed()));
    // Also listen to toggle so radio buttons are covered.
    connect(ui.buttonGroupProceed, &QButtonGroup::idClicked, this, &BugzillaReportConfirmationDialog::checkProceed);
    connect(ui.buttonGroupProceedQuestion, &QButtonGroup::idClicked, this, &BugzillaReportConfirmationDialog::checkProceed);

    if (!m_showProceedQuestion) {
        ui.proceedLabel->setEnabled(false);
        ui.proceedRadioYes->setEnabled(false);
        ui.proceedRadioNo->setEnabled(false);

        ui.proceedLabel->setVisible(false);
        ui.proceedRadioYes->setVisible(false);
        ui.proceedRadioNo->setVisible(false);

        ui.proceedRadioYes->setChecked(true);
    }

    checkProceed();
}

void BugzillaReportConfirmationDialog::checkProceed()
{
    bool yes = ui.proceedRadioYes->isChecked();
    bool no = ui.proceedRadioNo->isChecked();

    // Enable/disable labels and controls
    ui.areYouSureLabel->setEnabled(yes);
    ui.markAsDuplicateCheck->setEnabled(yes);
    ui.attachToBugReportCheck->setEnabled(yes);

    // Enable Continue button if valid options are selected
    bool possibleDupe = ui.markAsDuplicateCheck->isChecked();
    bool attach = ui.attachToBugReportCheck->isChecked();
    bool enableContinueButton = yes ? (possibleDupe || attach) : no;
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enableContinueButton);
}

void BugzillaReportConfirmationDialog::proceedClicked()
{
    if (ui.proceedRadioYes->isChecked()) {
        if (ui.markAsDuplicateCheck->isChecked()) {
            m_parent->markAsDuplicate();
            hide();
        } else {
            m_parent->attachToBugReport();
            hide();
        }
    } else {
        hide();
        m_parent->cancelAssistant();
    }
}
