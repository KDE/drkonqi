/*******************************************************************
* Copyright 2019    Harald Sitter <sitter@kde.org>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************/

#ifndef ASSISTANTPAGE_BUGZILLA_VERSION_H
#define ASSISTANTPAGE_BUGZILLA_VERSION_H

#include <QWidget>

#include "reportassistantpage.h"

class ReportAssistantDialog;
namespace Ui {
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

private:
    Ui::BugzillaVersionPage *ui = nullptr;
    KPageWidgetItem *m_item = nullptr;
};

#endif // ASSISTANTPAGE_BUGZILLA_VERSION_H
