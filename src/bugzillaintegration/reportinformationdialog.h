/*******************************************************************
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************/

#pragma once

#include <QDialog>

#include "ui_assistantpage_conclusions_dialog.h"

class ReportInformationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReportInformationDialog(const QString &reportText);
    ~ReportInformationDialog() override;

private Q_SLOTS:
    void saveReport();

private:
    Ui::AssistantPageConclusionsDialog ui;
};
