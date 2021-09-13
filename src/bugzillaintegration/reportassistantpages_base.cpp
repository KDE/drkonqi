/*******************************************************************
 * reportassistantpages_base.cpp
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2009 A. L. Spehr <spehr@kde.org>
 * SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "reportassistantpages_base.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFontDatabase>

#include <KLocalizedString>
#include <KMessageBox>
#include <KToolInvocation>
#include <KWindowConfig>

#include "applicationdetailsexamples.h"
#include "backtracegenerator.h"
#include "backtracewidget.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqi_globals.h"
#include "parser/backtraceparser.h"
#include "reportinterface.h"

// BEGIN IntroductionPage

IntroductionPage::IntroductionPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
{
    ui.setupUi(this);
    ui.m_warningIcon->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(64, 64));
}

// END IntroductionPage

// BEGIN CrashInformationPage

CrashInformationPage::CrashInformationPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
{
    m_backtraceWidget = new BacktraceWidget(DrKonqi::debuggerManager()->backtraceGenerator(), this, true);
    connect(m_backtraceWidget, &BacktraceWidget::stateChanged, this, &CrashInformationPage::emitCompleteChanged);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_backtraceWidget);
    layout->addSpacing(10); // We need this for better usability until we get something better

    // If the backtrace was already fetched on the main dialog, save it.
    BacktraceGenerator *btGenerator = DrKonqi::debuggerManager()->backtraceGenerator();
    if (btGenerator->state() == BacktraceGenerator::Loaded) {
        BacktraceParser::Usefulness use = btGenerator->parser()->backtraceUsefulness();
        if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
            reportInterface()->setBacktrace(btGenerator->backtrace());
        }
    }
}

void CrashInformationPage::aboutToShow()
{
    m_backtraceWidget->generateBacktrace();
    m_backtraceWidget->highlightExtraDetailsLabel(false);
    emitCompleteChanged();
}

void CrashInformationPage::aboutToHide()
{
    BacktraceGenerator *btGenerator = DrKonqi::debuggerManager()->backtraceGenerator();
    BacktraceParser::Usefulness use = btGenerator->parser()->backtraceUsefulness();

    if (use != BacktraceParser::Useless && use != BacktraceParser::InvalidUsefulness) {
        reportInterface()->setBacktrace(btGenerator->backtrace());
    }
    reportInterface()->setFirstBacktraceFunctions(btGenerator->parser()->firstValidFunctions());
}

bool CrashInformationPage::isComplete()
{
    BacktraceGenerator *generator = DrKonqi::debuggerManager()->backtraceGenerator();
    return (generator->state() != BacktraceGenerator::NotLoaded && generator->state() != BacktraceGenerator::Loading);
}

bool CrashInformationPage::showNextPage()
{
    BacktraceParser::Usefulness use = DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();

    if (DrKonqi::ignoreQuality()) {
        return true;
    }

    if ((use == BacktraceParser::InvalidUsefulness || use == BacktraceParser::ProbablyUseless || use == BacktraceParser::Useless)
        && m_backtraceWidget->canInstallDebugPackages()) {
        if (KMessageBox::Yes
            == KMessageBox::questionYesNo(this,
                                          i18nc("@info",
                                                "This crash information is not useful enough, "
                                                "do you want to try to improve it? You will need "
                                                "to install some debugging packages."),
                                          i18nc("@title:window", "Crash Information is not useful enough"))) {
            m_backtraceWidget->highlightExtraDetailsLabel(true);
            m_backtraceWidget->focusImproveBacktraceButton();
            return false; // Cancel show next, to allow the user to write more
        } else {
            return true; // Allow to continue
        }
    } else {
        return true;
    }
}

// END CrashInformationPage

// BEGIN BugAwarenessPage

BugAwarenessPage::BugAwarenessPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
{
    ui.setupUi(this);

    ui.m_actionsInsideApp->setText(
        i18nc("@option:check kind of information the user can provide "
              "about the crash, %1 is the application name",
              "What I was doing when the application \"%1\" crashed",
              DrKonqi::crashedApplication()->name()));

    connect(ui.m_rememberGroup, &QButtonGroup::idClicked, this, &BugAwarenessPage::updateCheckBoxes);
    // Also listen to toggle so radio buttons are covered.
    connect(ui.m_rememberGroup, &QButtonGroup::idClicked, this, &BugAwarenessPage::updateCheckBoxes);

    ui.m_appSpecificDetailsExamplesWidget->setVisible(reportInterface()->appDetailsExamples()->hasExamples());
    ui.m_appSpecificDetailsExamples->setText(
        i18nc("@label examples about information the user can provide", "Examples: %1", reportInterface()->appDetailsExamples()->examples()));
    ui.m_appSpecificDetailsExamples->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));

    if (qEnvironmentVariableIsSet("DRKONQI_TEST_MODE")) {
        ui.m_rememberCrashSituationYes->setChecked(true);
        ui.m_reproducibleBox->setCurrentIndex(m_reproducibleIndex.key(ReportInterface::ReproducibleEverytime));
    }
}

void BugAwarenessPage::aboutToShow()
{
    updateCheckBoxes();
}

void BugAwarenessPage::aboutToHide()
{
    // Save data
    ReportInterface::Reproducible reproducible = ReportInterface::ReproducibleUnsure;
    reproducible = m_reproducibleIndex.value(ui.m_reproducibleBox->currentIndex());

    reportInterface()->setBugAwarenessPageData(ui.m_rememberCrashSituationYes->isChecked(),
                                               reproducible,
                                               ui.m_actionsInsideApp->isChecked(),
                                               ui.m_unusualSituation->isChecked(),
                                               ui.m_appSpecificDetails->isChecked());
}

void BugAwarenessPage::updateCheckBoxes()
{
    const bool rememberSituation = ui.m_rememberCrashSituationYes->isChecked();

    ui.m_reproducibleLabel->setEnabled(rememberSituation);
    ui.m_reproducibleBox->setEnabled(rememberSituation);

    ui.m_informationLabel->setEnabled(rememberSituation);
    ui.m_actionsInsideApp->setEnabled(rememberSituation);
    ui.m_unusualSituation->setEnabled(rememberSituation);

    ui.m_appSpecificDetails->setEnabled(rememberSituation);
    ui.m_appSpecificDetailsExamples->setEnabled(rememberSituation);
}

// END BugAwarenessPage
