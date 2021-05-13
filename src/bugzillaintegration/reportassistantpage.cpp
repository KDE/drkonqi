/*******************************************************************
 * reportassistantpage.cpp
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "reportassistantpage.h"
#include "reportinterface.h"

ReportAssistantPage::ReportAssistantPage(ReportAssistantDialog *parent)
    : QWidget(parent)
    , m_assistant(parent)
{
}

bool ReportAssistantPage::isComplete()
{
    return true;
}

bool ReportAssistantPage::isAppropriate()
{
    return true;
}

bool ReportAssistantPage::showNextPage()
{
    return true;
}

ReportInterface *ReportAssistantPage::reportInterface() const
{
    return m_assistant->reportInterface();
}

BugzillaManager *ReportAssistantPage::bugzillaManager() const
{
    return reportInterface()->bugzillaManager();
}

ReportAssistantDialog *ReportAssistantPage::assistant() const
{
    return m_assistant;
}

void ReportAssistantPage::emitCompleteChanged()
{
    Q_EMIT completeChanged(this, isComplete());
}
