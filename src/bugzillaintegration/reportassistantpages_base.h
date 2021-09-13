/*******************************************************************
 * reportassistantpages_base.h
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef REPORTASSISTANTPAGES__BASE__H
#define REPORTASSISTANTPAGES__BASE__H

#include <QDialog>
#include <QPointer>

#include "reportassistantdialog.h"
#include "reportassistantpage.h"
#include "reportinterface.h"

#include "ui_assistantpage_bugawareness.h"
#include "ui_assistantpage_conclusions.h"
#include "ui_assistantpage_conclusions_dialog.h"
#include "ui_assistantpage_introduction.h"

class BacktraceWidget;

/** Introduction page **/
class IntroductionPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit IntroductionPage(ReportAssistantDialog *);

private:
    Ui::AssistantPageIntroduction ui;
};

/** Backtrace page **/
class CrashInformationPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit CrashInformationPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;
    bool isComplete() override;
    bool showNextPage() override;

private:
    BacktraceWidget *m_backtraceWidget;
};

/** Bug Awareness page **/
class BugAwarenessPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugAwarenessPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

private Q_SLOTS:
    void updateCheckBoxes();

private:
    Ui::AssistantPageBugAwareness ui;
    const QHash<int, ReportInterface::Reproducible> m_reproducibleIndex{{0, ReportInterface::ReproducibleUnsure},
                                                                        {1, ReportInterface::ReproducibleNever},
                                                                        {2, ReportInterface::ReproducibleSometimes},
                                                                        {3, ReportInterface::ReproducibleEverytime}};
};

#endif
