// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

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

class DrKonqiWidgetsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DrKonqiWidgetsDialog(QObject *parent = nullptr);
    ~DrKonqiWidgetsDialog() override;

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
