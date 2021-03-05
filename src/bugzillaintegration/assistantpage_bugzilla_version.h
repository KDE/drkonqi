/*******************************************************************
 * SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************/

#ifndef ASSISTANTPAGE_BUGZILLA_VERSION_H
#define ASSISTANTPAGE_BUGZILLA_VERSION_H

#include <QWidget>

#include "reportassistantpage.h"

class ReportAssistantDialog;
namespace Ui
{
class BugzillaVersionPage;
}

class BugzillaVersionPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaVersionPage(ReportAssistantDialog *parent = nullptr);
    ~BugzillaVersionPage();

    KPageWidgetItem *item() const;
    virtual bool isComplete() override;
    virtual bool isAppropriate() override;

private:
    Ui::BugzillaVersionPage *ui = nullptr;
    KPageWidgetItem *m_item = nullptr;
    bool appropriate = true;
};

#endif // ASSISTANTPAGE_BUGZILLA_VERSION_H
