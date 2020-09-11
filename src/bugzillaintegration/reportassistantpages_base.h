/*******************************************************************
* reportassistantpages_base.h
* SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* SPDX-License-Identifier: GPL-2.0-or-later
*
******************************************************************/

#ifndef REPORTASSISTANTPAGES__BASE__H
#define REPORTASSISTANTPAGES__BASE__H

#include <QPointer>
#include <QDialog>

#include "reportassistantdialog.h"
#include "reportassistantpage.h"

#include "ui_assistantpage_introduction.h"
#include "ui_assistantpage_bugawareness.h"
#include "ui_assistantpage_conclusions.h"
#include "ui_assistantpage_conclusions_dialog.h"

class BacktraceWidget;

/** Introduction page **/
class IntroductionPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit IntroductionPage(ReportAssistantDialog *);

private:
    Ui::AssistantPageIntroduction   ui;
};

/** Backtrace page **/
class CrashInformationPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit CrashInformationPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;
    bool isComplete() override;
    bool showNextPage() override;

private:
    BacktraceWidget *        m_backtraceWidget;
};

/** Bug Awareness page **/
class BugAwarenessPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugAwarenessPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

private Q_SLOTS:
    void updateCheckBoxes();

private:
    Ui::AssistantPageBugAwareness   ui;
};

/** Conclusions page **/
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
    Ui::AssistantPageConclusions            ui;

    QPointer<QDialog>                       m_infoDialog;

    bool                                    m_isBKO;
    bool                                    m_needToReport;

Q_SIGNALS:
    void finished(bool);
};

class ReportInformationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReportInformationDialog(const QString & reportText);
    ~ReportInformationDialog() override;

private Q_SLOTS:
    void saveReport();

private:
    Ui::AssistantPageConclusionsDialog ui;
};

#endif
