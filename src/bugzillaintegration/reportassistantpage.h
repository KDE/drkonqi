/*******************************************************************
* reportassistantpage.h
* SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* SPDX-License-Identifier: GPL-2.0-or-later
*
******************************************************************/

#ifndef REPORTASSISTANTPAGE__H
#define REPORTASSISTANTPAGE__H

#include <QWidget>

#include "reportassistantdialog.h"

class BugzillaManager;

/** BASE interface which implements some signals, and
**  aboutTo(Show|Hide) functions (also reimplements QWizard behaviour) **/
class ReportAssistantPage: public QWidget
{
    Q_OBJECT

public:
    explicit ReportAssistantPage(ReportAssistantDialog * parent);

    /** Load the widget data if empty **/
    virtual void aboutToShow() {}
    /** Save the widget data **/
    virtual void aboutToHide() {}
    /** Tells the KAssistantDialog to enable the Next button **/
    virtual bool isComplete();

    /** Last time checks to see if you can turn the page **/
    virtual bool showNextPage();

    ReportInterface *reportInterface() const;
    BugzillaManager *bugzillaManager() const;
    ReportAssistantDialog * assistant() const;

public Q_SLOTS:
    void emitCompleteChanged();

Q_SIGNALS:
    /** Tells the KAssistantDialog that the isComplete function changed value **/
    void completeChanged(ReportAssistantPage*, bool);

private:
    ReportAssistantDialog * const m_assistant;
};

#endif
