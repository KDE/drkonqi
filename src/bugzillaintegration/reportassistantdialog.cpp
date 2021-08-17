/*******************************************************************
 * reportassistantdialog.cpp
 * SPDX-FileCopyrightText: 2009, 2010 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "reportassistantdialog.h"

#include <QCloseEvent>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>

#include "drkonqi.h"
#include "drkonqi_debug.h"

#include "backtracegenerator.h"
#include "debuggermanager.h"
#include "parser/backtraceparser.h"

#include "aboutbugreportingdialog.h"
#include "assistantpage_bugzilla_supported_entities.h"
#include "assistantpage_bugzilla_version.h"
#include "assistantpage_conclusion.h"
#include "crashedapplication.h"
#include "reportassistantpages_base.h"
#include "reportassistantpages_bugzilla.h"
#include "reportassistantpages_bugzilla_duplicates.h"
#include "reportinterface.h"

static const char KDE_BUGZILLA_DESCRIPTION[] = I18N_NOOP("the KDE Bug Tracking System");

ReportAssistantDialog::ReportAssistantDialog(QWidget *parent)
    : KAssistantDialog(parent)
    , m_aboutBugReportingDialog(nullptr)
    , m_reportInterface(new ReportInterface(this))
    , m_canClose(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    // Set window properties
    setWindowTitle(i18nc("@title:window", "Crash Reporting Assistant"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug")));

    connect(this, &ReportAssistantDialog::currentPageChanged, this, &ReportAssistantDialog::currentPageChanged_slot);
    connect(button(QDialogButtonBox::Help), &QPushButton::clicked, this, &ReportAssistantDialog::showHelp);

    // Create the assistant pages

    //-Introduction Page
    KConfigGroup group(KSharedConfig::openConfig(), "ReportAssistant");
    const bool skipIntroduction = group.readEntry("SkipIntroduction", false);

    if (!skipIntroduction) {
        auto *m_introduction = new IntroductionPage(this);

        auto *m_introductionPage = new KPageWidgetItem(m_introduction, QLatin1String(PAGE_INTRODUCTION_ID));
        m_pageWidgetMap.insert(QLatin1String(PAGE_INTRODUCTION_ID), m_introductionPage);
        m_introductionPage->setHeader(i18nc("@title", "Welcome to the Reporting Assistant"));
        m_introductionPage->setIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug")));
        m_pageItems.push_back(m_introductionPage);
    }

    // Version check page
    auto *versionPage = new BugzillaVersionPage(this);
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZVERSION_ID), versionPage->item());

    // Product and version are valid page
    auto *entityPage = new BugzillaSupportedEntitiesPage(this);
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZENTITIES_ID), entityPage->item());

    //-Bug Awareness Page
    auto *m_awareness = new BugAwarenessPage(this);
    connectSignals(m_awareness);

    auto *m_awarenessPage = new KPageWidgetItem(m_awareness, QLatin1String(PAGE_AWARENESS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_AWARENESS_ID), m_awarenessPage);
    m_awarenessPage->setHeader(i18nc("@title", "What do you know about the crash?"));
    m_awarenessPage->setIcon(QIcon::fromTheme(QStringLiteral("checkbox")));

    //-Crash Information Page
    auto *m_backtrace = new CrashInformationPage(this);
    connectSignals(m_backtrace);

    auto *m_backtracePage = new KPageWidgetItem(m_backtrace, QLatin1String(PAGE_CRASHINFORMATION_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_CRASHINFORMATION_ID), m_backtracePage);
    m_backtracePage->setHeader(i18nc("@title", "Fetching the Backtrace (Automatic Crash Information)"));
    m_backtracePage->setIcon(QIcon::fromTheme(QStringLiteral("run-build")));

    //-Results Page
    auto *m_conclusions = new ConclusionPage(this);
    connectSignals(m_conclusions);

    auto *m_conclusionsPage = new KPageWidgetItem(m_conclusions, QLatin1String(PAGE_CONCLUSIONS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_CONCLUSIONS_ID), m_conclusionsPage);
    m_conclusionsPage->setHeader(i18nc("@title", "Results of the Analyzed Crash Details"));
    m_conclusionsPage->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
    connect(m_conclusions, &ConclusionPage::finished, this, &ReportAssistantDialog::assistantFinished);

    //-Bugzilla Login
    auto *m_bugzillaLogin = new BugzillaLoginPage(this);
    connectSignals(m_bugzillaLogin);

    auto *m_bugzillaLoginPage = new KPageWidgetItem(m_bugzillaLogin, QLatin1String(PAGE_BZLOGIN_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZLOGIN_ID), m_bugzillaLoginPage);
    m_bugzillaLoginPage->setHeader(i18nc("@title", "Login into %1", i18n(KDE_BUGZILLA_DESCRIPTION)));
    m_bugzillaLoginPage->setIcon(QIcon::fromTheme(QStringLiteral("user-identity")));
    connect(m_bugzillaLogin, &BugzillaLoginPage::loggedTurnToNextPage, this, &ReportAssistantDialog::loginFinished);

    //-Bugzilla duplicates
    auto *m_bugzillaDuplicates = new BugzillaDuplicatesPage(this);
    connectSignals(m_bugzillaDuplicates);

    auto *m_bugzillaDuplicatesPage = new KPageWidgetItem(m_bugzillaDuplicates, QLatin1String(PAGE_BZDUPLICATES_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZDUPLICATES_ID), m_bugzillaDuplicatesPage);
    m_bugzillaDuplicatesPage->setHeader(i18nc("@title", "Look for Possible Duplicate Reports"));
    m_bugzillaDuplicatesPage->setIcon(QIcon::fromTheme(QStringLiteral("repository")));

    //-Bugzilla information
    auto *m_bugzillaInformation = new BugzillaInformationPage(this);
    connectSignals(m_bugzillaInformation);

    auto *m_bugzillaInformationPage = new KPageWidgetItem(m_bugzillaInformation, QLatin1String(PAGE_BZDETAILS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZDETAILS_ID), m_bugzillaInformationPage);
    m_bugzillaInformationPage->setHeader(i18nc("@title", "Enter the Details about the Crash"));
    m_bugzillaInformationPage->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));

    //-Bugzilla Report Preview
    auto *m_bugzillaPreview = new BugzillaPreviewPage(this);

    auto *m_bugzillaPreviewPage = new KPageWidgetItem(m_bugzillaPreview, QLatin1String(PAGE_BZPREVIEW_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZPREVIEW_ID), m_bugzillaPreviewPage);
    m_bugzillaPreviewPage->setHeader(i18nc("@title", "Preview the Report"));
    m_bugzillaPreviewPage->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));

    //-Bugzilla commit
    auto *m_bugzillaSend = new BugzillaSendPage(this);

    auto *m_bugzillaSendPage = new KPageWidgetItem(m_bugzillaSend, QLatin1String(PAGE_BZSEND_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZSEND_ID), m_bugzillaSendPage);
    m_bugzillaSendPage->setHeader(i18nc("@title", "Sending the Crash Report"));
    m_bugzillaSendPage->setIcon(QIcon::fromTheme(QStringLiteral("applications-internet")));
    connect(m_bugzillaSend, &BugzillaSendPage::finished, this, &ReportAssistantDialog::assistantFinished);

    // Need to append because the welcome page is conditionally pushed early on.
    m_pageItems.insert(m_pageItems.end(),
                       {versionPage->item(),
                        entityPage->item(),
                        m_awarenessPage,
                        m_backtracePage,
                        m_conclusionsPage,
                        m_bugzillaLoginPage,
                        m_bugzillaDuplicatesPage,
                        m_bugzillaInformationPage,
                        m_bugzillaPreviewPage,
                        m_bugzillaSendPage});

    for (const auto pageItem : m_pageItems) {
        addPage(pageItem);
    }

    // Force a 16:9 ratio for nice appearance by default.
    QSize aspect(16, 9);
    aspect.scale(sizeHint(), Qt::KeepAspectRatioByExpanding);
    resize(aspect);
}

ReportAssistantDialog::~ReportAssistantDialog() = default;

void ReportAssistantDialog::setAboutToSend(bool aboutTo)
{
    if (aboutTo) {
        m_nextButtonIconCache = nextButton()->icon();
        m_nextButtonTextCache = nextButton()->text();
        nextButton()->setIcon(QIcon::fromTheme(QStringLiteral("document-send")));
        nextButton()->setText(i18nc("@action button to submit report", "Submit"));
        return;
    }
    nextButton()->setIcon(m_nextButtonIconCache);
    nextButton()->setText(m_nextButtonTextCache);
    m_nextButtonIconCache = QIcon();
    m_nextButtonTextCache = QString();
}

void ReportAssistantDialog::connectSignals(ReportAssistantPage *page)
{
    // React to the changes in the assistant pages
    connect(page, &ReportAssistantPage::completeChanged, this, &ReportAssistantDialog::completeChanged);
}

void ReportAssistantDialog::currentPageChanged_slot(KPageWidgetItem *current, KPageWidgetItem *before)
{
    // Page changed
    buttonBox()->button(QDialogButtonBox::Cancel)->setEnabled(true);
    m_canClose = false;

    qCDebug(DRKONQI_LOG) << "moving from" << before << (before ? before->name() : QString()) << "to" << current << (current ? current->name() : QString());

    // Save data of the previous page
    if (before) {
        auto *beforePage = qobject_cast<ReportAssistantPage *>(before->widget());
        beforePage->aboutToHide();
    }

    // Load data of the current(new) page
    if (current) {
        auto *currentPage = qobject_cast<ReportAssistantPage *>(current->widget());

        if (!currentPage->isAppropriate()) {
            // The page is inappropriate. Find where to go next. This is extra exhausting because
            // we can't just find the next or previous appropriate page because next() and back() implement lots of
            // bespoke movement rules. So we in fact constantly need to go through those functions and then check again here.
            // WARNING: when no page is appropriate this produces an ininfinite loop. There is no way to
            //   prevent this right now. next() and back() would need to stop hacking in shortcuts for us to reliably
            //   determine where to go and when we can't go anywhere.
            enum class Movement { Unknown, Forward, Backward };
            auto move = Movement::Unknown;
            for (auto it = m_pageItems.cbegin(); it != m_pageItems.cend(); ++it) {
                if (*it == before && move == Movement::Unknown) {
                    // Found previous item first, we are moving forward ->
                    move = Movement::Forward;
                    continue; // continue finding current item, depending on where it is in the container we may need to reverse
                }
                if (*it == current) {
                    if (move == Movement::Unknown) {
                        // Current item first, we are moving backward <-
                        move = Movement::Backward;
                        if (it == m_pageItems.cbegin()) {
                            // Turn around if this item is first but inappropriate.
                            move = Movement::Forward;
                        }
                    } else if ((it + 1) == m_pageItems.cend()) {
                        // Turn around if this item is last but inappropriate.
                        move = Movement::Backward;
                    }
                    break;
                }
            }

            qCDebug(DRKONQI_LOG) << "page inappropriate, skipping";

            switch (move) {
            case Movement::Unknown:
                qCDebug(DRKONQI_LOG) << "unknown";
                Q_UNREACHABLE();
                Q_FALLTHROUGH(); // do whatever at this point but do something, the page is inappropriate.
            case Movement::Forward:
                qCDebug(DRKONQI_LOG) << "forward";
                next();
                return;
            case Movement::Backward:
                qCDebug(DRKONQI_LOG) << "backward";
                back();
                return;
            }
            return;
        }

        nextButton()->setEnabled(currentPage->isComplete());
        currentPage->aboutToShow();

        // If the current page is the last one, disable all the buttons until the bug is sent
        if (current->name() == QLatin1String(PAGE_BZSEND_ID)) {
            nextButton()->setEnabled(false);
            backButton()->setEnabled(false);
            finishButton()->setEnabled(false);
        }
    }
}

void ReportAssistantDialog::completeChanged(ReportAssistantPage *page, bool isComplete)
{
    if (page == qobject_cast<ReportAssistantPage *>(currentPage()->widget())) {
        nextButton()->setEnabled(isComplete);
    }
}

void ReportAssistantDialog::assistantFinished(bool showBack)
{
    // The assistant finished: allow the user to close the dialog normally

    nextButton()->setEnabled(false);
    backButton()->setEnabled(showBack);
    finishButton()->setEnabled(true);
    buttonBox()->button(QDialogButtonBox::Cancel)->setEnabled(false);

    m_canClose = true;
}

void ReportAssistantDialog::loginFinished()
{
    // Bugzilla login finished, go to the next page
    if (currentPage()->name() == QLatin1String(PAGE_BZLOGIN_ID)) {
        next();
    }
}

void ReportAssistantDialog::showHelp()
{
    // Show the bug reporting guide dialog
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog();
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->raise();
    m_aboutBugReportingDialog->activateWindow();
    m_aboutBugReportingDialog->showSection(QLatin1String(PAGE_HELP_BEGIN_ID));
    m_aboutBugReportingDialog->showSection(currentPage()->name());
}

// Override KAssistantDialog "next" page implementation
void ReportAssistantDialog::next()
{
    // FIXME: this entire function is a bit weird. It'd likely make more sense to
    // use the page appropriateness more globally. i.e. mark pages inappropriate
    // when they are not applicable based on earlier settings done (e.g. put
    // a conclusion page under/after the awareness page but only mark it
    // appropriate if the data is not useful. that way kassistantdialog would
    // just skip over the page).

    // Allow the widget to Ask a question to the user before changing the page
    auto *page = qobject_cast<ReportAssistantPage *>(currentPage()->widget());
    if (page && !page->showNextPage()) {
        return;
    }

    const QString name = currentPage()->name();

    // If the information the user can provide is not useful, skip the backtrace page
    if (name == QLatin1String(PAGE_AWARENESS_ID)) {
        // Force save settings in the current page
        page->aboutToHide();

        if (!(m_reportInterface->isBugAwarenessPageDataUseful())) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CONCLUSIONS_ID)));
            return;
        }
    } else if (name == QLatin1String(PAGE_CRASHINFORMATION_ID)) {
        // Force save settings in current page
        page->aboutToHide();

        // Qt aborts clients when the wayland compositor crashes. We can't do anything with these reports and immediately
        // jump to the conclusion page. Additional handling happens there.
        if (DrKonqi::debuggerManager()->backtraceGenerator()->parser()->hasCompositorCrashed()) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CONCLUSIONS_ID)));
            return;
        }

        // If the crash is worth reporting and it is BKO, skip the Conclusions page
        if (m_reportInterface->isWorthReporting() && DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla()) {
            // Depending on whether the page is appropriate either go to version
            // check page or login page.
            const auto loginPage = m_pageWidgetMap.value(QLatin1String(PAGE_BZLOGIN_ID));
            setCurrentPage(loginPage);
            return;
        }
    } else if (name == QLatin1String(PAGE_BZDUPLICATES_ID)) {
        // a duplicate has been found, yet the report is not being attached
        if (m_reportInterface->duplicateId() && !m_reportInterface->attachToBugNumber()) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CONCLUSIONS_ID)));
            return;
        }
    }

    KAssistantDialog::next();
}

// Override KAssistantDialog "back"(previous) page implementation
// It has to mirror the custom next() implementation
void ReportAssistantDialog::back()
{
    if (currentPage()->name() == QLatin1String(PAGE_CONCLUSIONS_ID)) {
        if (m_reportInterface->duplicateId() && !m_reportInterface->attachToBugNumber()) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_BZDUPLICATES_ID)));
            return;
        }
        if (!(m_reportInterface->isBugAwarenessPageDataUseful())) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_AWARENESS_ID)));
            return;
        }
    }

    if (currentPage()->name() == QLatin1String(PAGE_BZLOGIN_ID)) {
        if (m_reportInterface->isWorthReporting() && DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla()) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CRASHINFORMATION_ID)));
            return;
        }
    }

    KAssistantDialog::back();
}

void ReportAssistantDialog::reject()
{
    close();
}

void ReportAssistantDialog::closeEvent(QCloseEvent *event)
{
    // Handle the close event
    if (!m_canClose) {
        // If the assistant didn't finished yet, offer the user the possibilities to
        // Close, Cancel, or Save the bug report and Close"

        KGuiItem closeItem = KStandardGuiItem::close();
        closeItem.setText(i18nc("@action:button", "Close the assistant"));

        KGuiItem keepOpenItem = KStandardGuiItem::cancel();
        keepOpenItem.setText(i18nc("@action:button", "Cancel"));

        BacktraceParser::Usefulness use = DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();
        if (use == BacktraceParser::ReallyUseful || use == BacktraceParser::MayBeUseful) {
            // Backtrace is still useful, let the user save it.
            KGuiItem saveBacktraceItem = KStandardGuiItem::save();
            saveBacktraceItem.setText(i18nc("@action:button", "Save information and close"));

            int ret = KMessageBox::questionYesNoCancel(this,
                                                       xi18nc("@info",
                                                              "Do you really want to close the bug reporting assistant? "
                                                              "<note>The crash information is still valid, so "
                                                              "you can save the report before closing if you want.</note>"),
                                                       i18nc("@title:window", "Close the Assistant"),
                                                       closeItem,
                                                       saveBacktraceItem,
                                                       keepOpenItem,
                                                       QString(),
                                                       KMessageBox::Dangerous);
            if (ret == KMessageBox::Yes) {
                event->accept();
            } else if (ret == KMessageBox::No) {
                // Save backtrace and accept event (dialog will be closed)
                DrKonqi::saveReport(reportInterface()->generateReportFullText(ReportInterface::DrKonqiStamp::Exclude, ReportInterface::Backtrace::Complete));
                event->accept();
            } else {
                event->ignore();
            }
        } else {
            if (KMessageBox::questionYesNo(this,
                                           i18nc("@info",
                                                 "Do you really want to close the bug "
                                                 "reporting assistant?"),
                                           i18nc("@title:window", "Close the Assistant"),
                                           closeItem,
                                           keepOpenItem,
                                           QString(),
                                           KMessageBox::Dangerous)
                == KMessageBox::Yes) {
                event->accept();
            } else {
                event->ignore();
            }
        }
    } else {
        event->accept();
    }
}

bool ReportAssistantDialog::isItemAppropriate(KPageWidgetItem *item) const
{
    // The base classes isAppropriate is not suitable for what we want, so we have a separate appropriateness system.
    // This function helps with getting our appropriateness value. It's named differently from the same function
    // in the base class, so we can at least try and intercept incorrect calls to that other function. It's hit and
    // miss though since the function isn't virtual. Alas.
    if (!item) {
        return false;
    }
    auto page = qobject_cast<ReportAssistantPage *>(item->widget());
    return page && page->isAppropriate();
}
