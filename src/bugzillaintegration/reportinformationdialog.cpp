/*******************************************************************
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2009 A. L. Spehr <spehr@kde.org>
 * SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************/

#include "reportinformationdialog.h"

#include <QPushButton>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>

#include "drkonqi_globals.h"

ReportInformationDialog::ReportInformationDialog(const QString &reportText)
    : QDialog()
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(i18nc("@title:window", "Contents of the Report"));

    ui.setupUi(this);
    ui.m_reportInformationBrowser->setPlainText(reportText);

    auto *saveButton = new QPushButton(ui.buttonBox);
    KGuiItem::assign(saveButton,
                     KGuiItem2(i18nc("@action:button", "&Save to File..."),
                               QIcon::fromTheme(QStringLiteral("document-save")),
                               i18nc("@info:tooltip",
                                     "Use this button to save the "
                                     "generated crash report information to "
                                     "a file. You can use this option to report the "
                                     "bug later.")));
    connect(saveButton, &QPushButton::clicked, this, &ReportInformationDialog::saveReport);
    ui.buttonBox->addButton(saveButton, QDialogButtonBox::ActionRole);

    KConfigGroup config(KSharedConfig::openConfig(), "ReportInformationDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), config);
}

ReportInformationDialog::~ReportInformationDialog()
{
    KConfigGroup config(KSharedConfig::openConfig(), "ReportInformationDialog");
    KWindowConfig::saveWindowSize(windowHandle(), config);
}

void ReportInformationDialog::saveReport()
{
    DrKonqi::saveReport(ui.m_reportInformationBrowser->toPlainText(), this);
}
