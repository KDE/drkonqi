/*******************************************************************
 * reportassistantdialog.h
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

class ReportAssistantDialog : public KAssistantDialog
{
    Q_OBJECT

public:
    explicit ReportAssistantDialog(QWidget *parent = nullptr);
    ~ReportAssistantDialog() override;

    ReportInterface *reportInterface() const
    {
        return m_reportInterface;
    }

    void setAboutToSend(bool aboutTo);

public Q_SLOTS:
    void next() override;
    void back() override;

    void assistantFinished(bool);

private Q_SLOTS:
    void currentPageChanged_slot(KPageWidgetItem *, KPageWidgetItem *);

    void completeChanged(ReportAssistantPage *, bool);

    void loginFinished();


    void showHelp();

    // Override default reject method
    void reject() override;

private:
    void connectSignals(ReportAssistantPage *);
    void closeEvent(QCloseEvent *) override;

    QHash<QLatin1String, KPageWidgetItem *> m_pageWidgetMap;

    QPointer<AboutBugReportingDialog> m_aboutBugReportingDialog;
    ReportInterface *m_reportInterface = nullptr;

    bool m_canClose;

    QIcon m_nextButtonIconCache;
    QString m_nextButtonTextCache;
    // Page sequence.
    std::vector<KPageWidgetItem *> m_pageItems;
};

#endif
