/*******************************************************************
* drkonqidialog.h
* SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* SPDX-License-Identifier: GPL-2.0-or-later
*
******************************************************************/

#ifndef DRKONQIDIALOG__H
#define DRKONQIDIALOG__H

#include <QPointer>
#include <QHash>

#include <QDialog>

#include "ui_maindialog.h"

class BacktraceWidget;
class AboutBugReportingDialog;
class QTabWidget;
class AbstractDebuggerLauncher;
class QDialogButtonBox;
class QMenu;

class DrKonqiDialog: public QDialog
{
    Q_OBJECT

public:
    explicit DrKonqiDialog(QWidget * parent = nullptr);
    ~DrKonqiDialog() override;

private Q_SLOTS:
    void linkActivated(const QString&);
    void startBugReportAssistant();

    void applicationRestarted(bool success);

    void addDebugger(AbstractDebuggerLauncher *launcher);
    void removeDebugger(AbstractDebuggerLauncher *launcher);
    void enableDebugMenu(bool);

    //GUI
    void buildIntroWidget();
    void buildDialogButtons();

    void tabIndexChanged(int);

private:
    void showAboutBugReporting();

    QTabWidget *                        m_tabWidget = nullptr;

    QPointer<AboutBugReportingDialog>   m_aboutBugReportingDialog;

    QWidget *                           m_introWidget = nullptr;
    Ui::MainWidget                      ui;

    BacktraceWidget *                   m_backtraceWidget = nullptr;

    QMenu *m_debugMenu;
    QHash<AbstractDebuggerLauncher*, QAction*> m_debugMenuActions;
    QDialogButtonBox*                   m_buttonBox = nullptr;
    QPushButton*                        m_debugButton = nullptr;
    QPushButton*                        m_restartButton = nullptr;
    bool                                m_debugButtonInBox = false;
};

#endif
