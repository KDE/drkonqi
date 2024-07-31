/*******************************************************************
 * drkonqidialog.h
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef DRKONQIDIALOG__H
#define DRKONQIDIALOG__H

#include <QHash>
#include <QPointer>

#include <QDialog>

#include "ui_maindialog.h"

class BacktraceWidget;
class QTabWidget;
class QDialogButtonBox;
class QMenu;

class DrKonqiDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DrKonqiDialog(QWidget *parent = nullptr);
    ~DrKonqiDialog() override;

    enum class GoTo { Main, Sentry };
    void show(GoTo to);

private Q_SLOTS:
    void linkActivated(const QString &);

    void applicationRestarted(bool success);

    // GUI
    void buildIntroWidget();
    void buildDialogButtons();

    void tabIndexChanged(int);

private:
    QTabWidget *m_tabWidget = nullptr;

    QWidget *m_introWidget = nullptr;
    Ui::MainWidget ui;

    BacktraceWidget *m_backtraceWidget = nullptr;

    QDialogButtonBox *m_buttonBox = nullptr;
    QPushButton *m_restartButton = nullptr;
    bool m_debugButtonInBox = false;
};

#endif
