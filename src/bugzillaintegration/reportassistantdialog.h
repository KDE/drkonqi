/*******************************************************************
* reportassistantdialog.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2019 Harald Sitter <sitter@kde.org>
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
*
******************************************************************/

#ifndef REPORTASSISTANTDIALOG__H
#define REPORTASSISTANTDIALOG__H

#include <QIcon>
#include <QPointer>

#include <KAssistantDialog>

class ReportAssistantPage;
class AboutBugReportingDialog;
class ReportInterface;
class QCloseEvent;

class ReportAssistantDialog: public KAssistantDialog
{
    Q_OBJECT

public:
    explicit ReportAssistantDialog(QWidget * parent = nullptr);
    ~ReportAssistantDialog() override;

    ReportInterface *reportInterface() const {
        return m_reportInterface;
    }

    void setAboutToSend(bool aboutTo);

public Q_SLOTS:
    void next() override;
    void back() override;

private Q_SLOTS:
    void currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *);

    void completeChanged(ReportAssistantPage*, bool);

    void loginFinished();

    void assistantFinished(bool);

    void showHelp();

    //Override default reject method
    void reject() override;

private:
    void connectSignals(ReportAssistantPage *);
    void closeEvent(QCloseEvent*) override;

    QHash<QLatin1String, KPageWidgetItem*>       m_pageWidgetMap;

    QPointer<AboutBugReportingDialog>   m_aboutBugReportingDialog;
    ReportInterface *                m_reportInterface = nullptr;

    bool                        m_canClose;

    QIcon m_nextButtonIconCache;
    QString m_nextButtonTextCache;
};

#endif
