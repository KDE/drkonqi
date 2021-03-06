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

static QHash<int, ReportInterface::Reproducible> s_reproducibleIndex{{0, ReportInterface::ReproducibleUnsure},
                                                                     {1, ReportInterface::ReproducibleNever},
                                                                     {2, ReportInterface::ReproducibleSometimes},
                                                                     {3, ReportInterface::ReproducibleEverytime}};

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
        ui.m_reproducibleBox->setCurrentIndex(s_reproducibleIndex.key(ReportInterface::ReproducibleEverytime));
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
    reproducible = s_reproducibleIndex.value(ui.m_reproducibleBox->currentIndex());

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

// BEGIN ConclusionPage

ConclusionPage::ConclusionPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
    , m_needToReport(false)
{
    m_isBKO = DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla();

    ui.setupUi(this);

    KGuiItem::assign(ui.m_showReportInformationButton,
                     KGuiItem2(i18nc("@action:button", "&Show Contents of the Report"),
                               QIcon::fromTheme(QStringLiteral("document-preview")),
                               i18nc("@info:tooltip",
                                     "Use this button to show the generated "
                                     "report information about this crash.")));
    connect(ui.m_showReportInformationButton, &QPushButton::clicked, this, &ConclusionPage::openReportInformation);

    connect(ui.m_explanationLabel, &QLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(QUrl(link));
    });

    ui.m_restartAppOnFinish->setVisible(false);
}

void ConclusionPage::finishClicked()
{
    // Manual report
    if (m_needToReport && !m_isBKO) {
        const CrashedApplication *crashedApp = DrKonqi::crashedApplication();
        BugReportAddress reportAddress = crashedApp->bugReportAddress();
        QString report = reportInterface()->generateReportFullText(ReportInterface::DrKonqiStamp::Exclude, ReportInterface::Backtrace::Complete);

        if (reportAddress.isEmail()) {
            QString subject = QStringLiteral("[%1] [%2] Automatic crash report generated by DrKonqi");
            subject = subject.arg(crashedApp->name());
            subject = subject.arg(crashedApp->datetime().toString(QStringLiteral("yyyy-MM-dd")));
            KToolInvocation::invokeMailer(reportAddress, QLatin1String(""), QLatin1String(""), subject, report);
        } else {
            QUrl url(reportAddress);
            if (QUrl(reportAddress).isRelative()) { // Scheme is missing
                url = QUrl(QString::fromLatin1("https://%1").arg(reportAddress));
            }
            QDesktopServices::openUrl(url);
        }

        // Show a copy of the bug reported
        openReportInformation();
    }

    // Restart application
    if (ui.m_restartAppOnFinish->isChecked()) {
        DrKonqi::crashedApplication()->restart();
    }
}

void ConclusionPage::aboutToShow()
{
    connect(assistant()->finishButton(), &QPushButton::clicked, this, &ConclusionPage::finishClicked);
    ui.m_restartAppOnFinish->setVisible(false);
    ui.m_restartAppOnFinish->setChecked(false);

    const bool isDuplicate = reportInterface()->duplicateId() && !reportInterface()->attachToBugNumber();
    m_needToReport = reportInterface()->isWorthReporting() && !isDuplicate;
    emitCompleteChanged();

    if (DrKonqi::debuggerManager()->backtraceGenerator()->parser()->hasCompositorCrashed()) {
        ui.m_conclusionsLabel->setText(i18nc("@info",
                                             "It appears that this crash is actually the result of an earlier crash in the compositor and cannot be "
                                             "submitted as it lacks the required information to resolve the problem."));
        ui.m_explanationLabel->setText(xi18nc(
            "@info",
            "KDE developers would appreciate if you could find and attach the crash log for the compositor. You may be able to retrieve it using the "
            "coredumpctl utility. For more information, see "
            "<link "
            "url='https://community.kde.org/Guidelines_and_HOWTOs/Debugging/How_to_create_useful_crash_reports#Retrieving_a_backtrace_using_coredumpctl'>the "
            "KDE Community Wiki</"
            "link>."));
        ui.m_showReportInformationButton->setVisible(false); // Don't give access to the report, lest people file them manually.
        if (!DrKonqi::crashedApplication()->hasBeenRestarted()) {
            ui.m_restartAppOnFinish->setVisible(true);
        }
        Q_EMIT finished(false /* don't enable back button - the user can't improve this result*/);
        return;
    }

    BugReportAddress reportAddress = DrKonqi::crashedApplication()->bugReportAddress();
    BacktraceParser::Usefulness use = DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();

    QString explanationHTML = QLatin1String("<p><ul>");

    bool backtraceGenerated = true;
    switch (use) {
    case BacktraceParser::ReallyUseful: {
        explanationHTML += QStringLiteral("<li>%1</li>")
                               .arg(i18nc("@info",
                                          "The automatically generated "
                                          "crash information is useful."));
        break;
    }
    case BacktraceParser::MayBeUseful: {
        explanationHTML += QStringLiteral("<li>%1</li>")
                               .arg(i18nc("@info",
                                          "The automatically generated "
                                          "crash information lacks some "
                                          "details "
                                          "but may be still be useful."));
        break;
    }
    case BacktraceParser::ProbablyUseless: {
        explanationHTML += QStringLiteral("<li>%1</li>")
                               .arg(i18nc("@info",
                                          "The automatically generated "
                                          "crash information lacks important details "
                                          "and it is probably not helpful."));
        break;
    }
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        BacktraceGenerator::State state = DrKonqi::debuggerManager()->backtraceGenerator()->state();
        if (state == BacktraceGenerator::NotLoaded) {
            backtraceGenerated = false;
            explanationHTML += QStringLiteral("<li>%1</li>")
                                   .arg(i18nc("@info",
                                              "The crash information was "
                                              "not generated because it was not needed."));
        } else {
            explanationHTML += QStringLiteral("<li>%1<br />%2</li>")
                                   .arg(i18nc("@info",
                                              "The automatically generated crash "
                                              "information does not contain enough information to be "
                                              "helpful."),
                                        xi18nc("@info",
                                               "<note>You can improve it by "
                                               "installing debugging packages and reloading the crash on "
                                               "the Crash Information page. You can get help with the Bug "
                                               "Reporting Guide by clicking on the "
                                               "<interface>Help</interface> button.</note>"));
            // but this guide doesn't mention bt packages? that's techbase
            //->>and the help guide mention techbase page...
        }
        break;
    }
    }

    // User can provide enough information
    if (reportInterface()->isBugAwarenessPageDataUseful()) {
        explanationHTML += QStringLiteral("<li>%1</li>")
                               .arg(i18nc("@info",
                                          "The information you can "
                                          "provide could be considered helpful."));
    } else {
        explanationHTML += QStringLiteral("<li>%1</li>")
                               .arg(i18nc("@info",
                                          "The information you can "
                                          "provide is not considered helpful enough in this case."));
    }

    if (isDuplicate) {
        explanationHTML += QStringLiteral("<li>%1</li>")
                               .arg(xi18nc("@info",
                                           "Your problem has already been "
                                           "reported as bug %1.",
                                           QString::number(reportInterface()->duplicateId())));
    }

    explanationHTML += QLatin1String("</ul></p>");

    ui.m_explanationLabel->setText(explanationHTML);

    // Hide the "Show contents of the report" button if the backtrace was not generated
    ui.m_showReportInformationButton->setVisible(backtraceGenerated);

    if (m_needToReport) {
        ui.m_conclusionsLabel->setText(QStringLiteral("<p><strong>%1</strong>")
                                           .arg(i18nc("@info",
                                                      "This "
                                                      "report is considered helpful.")));

        if (m_isBKO) {
            emitCompleteChanged();
            ui.m_howToProceedLabel->setText(xi18nc("@info",
                                                   "This application's bugs are reported "
                                                   "to the KDE bug tracking system: click <interface>Next"
                                                   "</interface> to start the reporting process. "
                                                   "You can manually report at <link>%1</link>",
                                                   reportAddress));

        } else {
            if (!DrKonqi::crashedApplication()->hasBeenRestarted()) {
                ui.m_restartAppOnFinish->setVisible(true);
            }

            ui.m_howToProceedLabel->setText(xi18nc("@info",
                                                   "This application is not supported in the "
                                                   "KDE bug tracking system. Click <interface>"
                                                   "Finish</interface> to report this bug to "
                                                   "the application maintainer. Also, you can manually "
                                                   "report at <link>%1</link>.",
                                                   reportAddress));

            Q_EMIT finished(false);
        }

    } else { // (m_needToReport)
        if (!DrKonqi::crashedApplication()->hasBeenRestarted()) {
            ui.m_restartAppOnFinish->setVisible(true);
        }

        ui.m_conclusionsLabel->setText(QStringLiteral("<p><strong>%1</strong><br />%2</p>")
                                           .arg(i18nc("@info",
                                                      "This report does not contain enough information for the "
                                                      "developers, so the automated bug reporting process is not "
                                                      "enabled for this crash."),
                                                i18nc("@info",
                                                      "If you wish, you can go back and change your "
                                                      "answers. ")));

        // Only mention "manual reporting" if the backtrace was generated.
        // FIXME separate the texts "manual reporting" / "click finish to close"
        //"manual reporting" should be ~"manual report using the contents of the report"....
        // FIXME for 4.5 (workflow, see ToDo)
        if (backtraceGenerated) {
            if (m_isBKO) {
                ui.m_howToProceedLabel->setText(xi18nc("@info",
                                                       "You can manually report this bug "
                                                       "at <link>%1</link>. "
                                                       "Click <interface>Finish</interface> to close the "
                                                       "assistant.",
                                                       reportAddress));
            } else {
                ui.m_howToProceedLabel->setText(xi18nc("@info",
                                                       "You can manually report this "
                                                       "bug to its maintainer at <link>%1</link>. "
                                                       "Click <interface>Finish</interface> to close the "
                                                       "assistant.",
                                                       reportAddress));
            }
        }
        Q_EMIT finished(true);
    }
}

void ConclusionPage::aboutToHide()
{
    disconnect(assistant()->finishButton(), &QPushButton::clicked, this, &ConclusionPage::finishClicked);
}

void ConclusionPage::openReportInformation()
{
    if (!m_infoDialog) {
        QString info = reportInterface()->generateReportFullText(ReportInterface::DrKonqiStamp::Exclude, ReportInterface::Backtrace::Complete)
            + QLatin1Char('\n') + i18nc("@info report to url/mail address", "Report to %1", DrKonqi::crashedApplication()->bugReportAddress());

        m_infoDialog = new ReportInformationDialog(info);
    }
    m_infoDialog->show();
    m_infoDialog->raise();
    m_infoDialog->activateWindow();
}

bool ConclusionPage::isComplete()
{
    return (m_isBKO && m_needToReport);
}

// END ConclusionPage

// BEGIN ReportInformationDialog

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

// END ReportInformationDialog
