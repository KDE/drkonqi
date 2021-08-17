/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <QWidget>

#include <memory>

#include "reportassistantpage.h"

class ReportAssistantDialog;

namespace Ui
{
class BugzillaSupportedEntitiesPage;
} // namespace Ui

class BugzillaSupportedEntitiesPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaSupportedEntitiesPage(ReportAssistantDialog *parent = nullptr);
    ~BugzillaSupportedEntitiesPage() override; // for the unique_ptr to get away with forward-decl

    KPageWidgetItem *item() const;
    bool isComplete() override;
    bool isAppropriate() override;
    void aboutToShow() override;

private:
    void finishAssistant();
    bool isCurrentPage() const;
    void errorOut();

    std::unique_ptr<Ui::BugzillaSupportedEntitiesPage> ui;
    KPageWidgetItem *m_item = nullptr; // not owned by us
    bool m_appropriate = true;

    Q_DISABLE_COPY_MOVE(BugzillaSupportedEntitiesPage) // rule of 5
};
