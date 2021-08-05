/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
*/

#include "assistantpage_bugzilla_supported_entities.h"

#include "bugzillalib.h"
#include "crashedapplication.h"
#include "drkonqi.h"
#include "productmapping.h"
#include "reportinterface.h"
#include "ui_assistantpage_bugzilla_supported_entities.h"

BugzillaSupportedEntitiesPage::BugzillaSupportedEntitiesPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
    , ui(new Ui::BugzillaSupportedEntitiesPage)
    , m_item(new KPageWidgetItem(this))
{
    if (!DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla()) {
        m_appropriate = false;
        return; // never appropriate as bugzilla doesn't matter
    }

    // This item is intentionally not titled. The page should usually not show
    // up when when it shows up it should focus on the bare essentials!
    m_item->setIcon(QIcon::fromTheme(QStringLiteral("outdated")));

    // We are not valid until the mapping resolved!
    assistant()->setValid(m_item, false);

    ui->setupUi(this);
    ui->errorIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("outdated")).pixmap(ui->errorIconLabel->size()));
    ui->errorLabel->setOpenExternalLinks(true);
    ui->errorWidget->hide();

    ProductMapping *mapping = reportInterface()->productMapping();
    connect(mapping, &ProductMapping::resolved, this, [this, mapping] {
        if (mapping->bugzillaProductDisabled()) {
            ui->errorLabel->setText(xi18nc("@info",
                                           "Thanks for wanting to help improve our software! Unfortunately <resource>%1</resource> is no longer supported. "
                                           "We may have alterantive supported software on offer on <link url='https://apps.kde.org/'>KDE.org</link>.",
                                           mapping->bugzillaProduct()));
            errorOut();
            return;
        }
        if (mapping->bugzillaVersionDisabled()) {
            ui->errorLabel->setText(
                xi18nc("@info",
                       "Thanks for wanting to help improve our software! Unfortunately version <resource>%1</resource> of <resource>%2</resource> is no longer "
                       "supported. Please upgrade to a newer version if possible. If your distribution does not provide a newer version you may be able to "
                       "find alternative installation options on <link url='https://apps.kde.org/'>KDE.org</link>.",
                       mapping->bugzillaVersion(),
                       mapping->bugzillaProduct()));
            errorOut();
            return;
        }

        m_appropriate = false;
        if (assistant()->currentPage() == m_item) {
            assistant()->next(); // skip ahead
        }
    });
}

BugzillaSupportedEntitiesPage::~BugzillaSupportedEntitiesPage() = default;

bool BugzillaSupportedEntitiesPage::isComplete()
{
    return false;
}

bool BugzillaSupportedEntitiesPage::isAppropriate()
{
    return m_appropriate;
}

KPageWidgetItem *BugzillaSupportedEntitiesPage::item() const
{
    return m_item;
}

void BugzillaSupportedEntitiesPage::aboutToShow()
{
    finishAssistant(); // if this page gets shown we had an error -> conclude the assistant
}

void BugzillaSupportedEntitiesPage::finishAssistant()
{
    assistant()->assistantFinished(false);
}

bool BugzillaSupportedEntitiesPage::isCurrentPage() const
{
    return assistant()->currentPage() == m_item;
}

void BugzillaSupportedEntitiesPage::errorOut()
{
    m_appropriate = true;
    ui->busyWidget->hide();
    ui->errorWidget->show();
    if (isCurrentPage()) {
        finishAssistant();
    }
}
