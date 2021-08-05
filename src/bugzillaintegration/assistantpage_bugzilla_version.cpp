/*******************************************************************
 * SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************/

#include "assistantpage_bugzilla_version.h"
#include "ui_assistantpage_bugzilla_version.h"

#include <QGuiApplication>

#include "bugzillalib.h"
#include "crashedapplication.h"
#include "drkonqi.h"

BugzillaVersionPage::BugzillaVersionPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
    , ui(new Ui::BugzillaVersionPage)
    , m_item(new KPageWidgetItem(this))
{
    if (!DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla()) {
        appropriate = false;
        return; // never appropriate as bugzilla doesn't matter
    }
    // This item is intentionally not titled. The page should usually not show
    // up when when it shows up it should focus on the bare essentials!
    m_item->setIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug")));

    // We are not valid until the version came back!
    assistant()->setValid(m_item, false);

    ui->setupUi(this);

    ui->errorIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("state-error")).pixmap(ui->errorIconLabel->size()));

    connect(bugzillaManager(), &BugzillaManager::bugzillaVersionFound, this, [=] {
        // Don't show this page ever again!
        appropriate = false;
        if (assistant()->currentPage() == m_item) {
            assistant()->next();
        }
    });
    connect(bugzillaManager(), &BugzillaManager::bugzillaVersionError, this, [=](const QString &error) {
        ui->busyWidget->hide();
        ui->errorWidget->show();
        ui->errorLabel->setText(xi18nc("@info %1 is an error message from the backend", "Failed to contact bugs.kde.org: <message>%1</message>", error));
    });
    connect(ui->retryButton, &QPushButton::clicked, this, [=] {
        ui->busyWidget->show();
        ui->errorWidget->hide();
        bugzillaManager()->lookupVersion();
    });

    // Finally trigger the actual load!
    ui->retryButton->click();
}

BugzillaVersionPage::~BugzillaVersionPage()
{
    delete ui;
}

bool BugzillaVersionPage::isComplete()
{
    return false;
}

bool BugzillaVersionPage::isAppropriate()
{
    return appropriate;
}

KPageWidgetItem *BugzillaVersionPage::item() const
{
    return m_item;
}
