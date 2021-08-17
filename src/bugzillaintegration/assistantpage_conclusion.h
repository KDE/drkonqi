/*******************************************************************
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************/

#pragma once

#include "reportassistantpage.h"

#include "ui_assistantpage_conclusions.h"

class ConclusionPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit ConclusionPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

    bool isComplete() override;

private Q_SLOTS:
    void finishClicked();

    void openReportInformation();

private:
    Ui::AssistantPageConclusions ui;

    QPointer<QDialog> m_infoDialog;

    bool m_isBKO;
    bool m_needToReport;

Q_SIGNALS:
    void finished(bool);
};
