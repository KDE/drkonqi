/*******************************************************************
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "bugzillareportinformationdialog.h"

#include <QDebug>

#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KWindowConfig>

#include "bugzillareportconfirmationdialog.h"
#include "drkonqi_globals.h"
#include "reportassistantpages_bugzilla_duplicates.h"
#include "reportinterface.h"

BugzillaReportInformationDialog::BugzillaReportInformationDialog(BugzillaDuplicatesPage *parent)
    : QDialog(parent)
    , m_relatedButtonEnabled(true)
    , m_parent(parent)
    , m_bugNumber(0)
    , m_duplicatesCount(0)
{
    setWindowTitle(i18nc("@title:window", "Bug Description"));

    ui.setupUi(this);

    KGuiItem::assign(ui.m_retryButton,
                     KGuiItem2(i18nc("@action:button", "Retry..."),
                               QIcon::fromTheme(QStringLiteral("view-refresh")),
                               i18nc("@info:tooltip",
                                     "Use this button to retry "
                                     "loading the bug report.")));
    connect(ui.m_retryButton, &QPushButton::clicked, this, &BugzillaReportInformationDialog::reloadReport);

    m_suggestButton = new QPushButton(this);
    ui.buttonBox->addButton(m_suggestButton, QDialogButtonBox::ActionRole);
    KGuiItem::assign(m_suggestButton,
                     KGuiItem2(i18nc("@action:button", "Suggest this crash is related"),
                               QIcon::fromTheme(QStringLiteral("list-add")),
                               i18nc("@info:tooltip",
                                     "Use this button to suggest that "
                                     "the crash you experienced is related to this bug "
                                     "report")));
    connect(m_suggestButton, &QPushButton::clicked, this, &BugzillaReportInformationDialog::relatedReportClicked);

    connect(ui.m_showOwnBacktraceCheckBox, &QAbstractButton::toggled, this, &BugzillaReportInformationDialog::toggleShowOwnBacktrace);

    // Connect bugzillalib signals
    connect(m_parent->bugzillaManager(), &BugzillaManager::bugReportFetched, this, &BugzillaReportInformationDialog::bugFetchFinished);
    connect(m_parent->bugzillaManager(), &BugzillaManager::bugReportError, this, &BugzillaReportInformationDialog::bugFetchError);
    connect(m_parent->bugzillaManager(), &BugzillaManager::commentsFetched, this, &BugzillaReportInformationDialog::onCommentsFetched);
    connect(m_parent->bugzillaManager(), &BugzillaManager::commentsError, this, &BugzillaReportInformationDialog::bugFetchError);

    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), config);
}

BugzillaReportInformationDialog::~BugzillaReportInformationDialog()
{
    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    KWindowConfig::saveWindowSize(windowHandle(), config);
}

void BugzillaReportInformationDialog::reloadReport()
{
    showBugReport(m_bugNumber);
}

void BugzillaReportInformationDialog::showBugReport(int bugNumber, bool relatedButtonEnabled)
{
    m_relatedButtonEnabled = relatedButtonEnabled;
    ui.m_retryButton->setVisible(false);

    m_closedStateString.clear();
    m_bugNumber = bugNumber;
    m_parent->bugzillaManager()->fetchBugReport(m_bugNumber, this);

    m_suggestButton->setEnabled(false);
    m_suggestButton->setVisible(m_relatedButtonEnabled);

    ui.m_infoBrowser->setText(i18nc("@info:status", "Loading..."));
    ui.m_infoBrowser->setEnabled(false);

    ui.m_linkLabel->setText(xi18nc("@info", "<link url='%1'>Report's webpage</link>", m_parent->bugzillaManager()->urlForBug(m_bugNumber)));

    ui.m_statusWidget->setBusy(xi18nc("@info:status",
                                      "Loading information about bug "
                                      "%1 from %2....",
                                      QString::number(m_bugNumber),
                                      QLatin1String(KDE_BUGZILLA_SHORT_URL)));

    ui.m_backtraceBrowser->setPlainText(i18nc("@info", "Backtrace of the crash I experienced:\n\n") + m_parent->reportInterface()->backtrace());

    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    bool showOwnBacktrace = config.readEntry("ShowOwnBacktrace", false);
    ui.m_showOwnBacktraceCheckBox->setChecked(showOwnBacktrace);
    if (!showOwnBacktrace) { // setChecked(false) will not Q_EMIT toggled(false)
        toggleShowOwnBacktrace(false);
    }

    show();
}

struct Status2 {
    QString statusString;
    QString closedStateString;
};

static Status2 statusString2(const Bugzilla::Bug::Ptr &bug)
{
    // Generate a non-geek readable status
    switch (bug->status()) {
    case Bugzilla::Bug::Status::UNCONFIRMED:
        return {i18nc("@info bug status", "Opened (Unconfirmed)"), QString()};
    case Bugzilla::Bug::Status::CONFIRMED:
    case Bugzilla::Bug::Status::ASSIGNED:
    case Bugzilla::Bug::Status::REOPENED:
        return {i18nc("@info bug status", "Opened (Unfixed)"), QString()};

    case Bugzilla::Bug::Status::RESOLVED:
    case Bugzilla::Bug::Status::VERIFIED:
    case Bugzilla::Bug::Status::CLOSED:
        switch (bug->resolution()) {
        case Bugzilla::Bug::Resolution::FIXED: {
            auto fixedIn = bug->customField("cf_versionfixedin").toString();
            if (!fixedIn.isEmpty()) {
                return {i18nc("@info bug resolution, fixed in version", "Fixed in version \"%1\"", fixedIn),
                        i18nc("@info bug resolution, fixed by kde devs in version", "the bug was fixed by KDE developers in version \"%1\"", fixedIn)};
            }
            return {i18nc("@info bug resolution", "Fixed"), i18nc("@info bug resolution", "the bug was fixed by KDE developers")};
        }

        case Bugzilla::Bug::Resolution::WORKSFORME:
            return {i18nc("@info bug resolution", "Non-reproducible"), QString()};
        case Bugzilla::Bug::Resolution::DUPLICATE:
            return {i18nc("@info bug resolution", "Duplicate report (Already reported before)"), QString()};
        case Bugzilla::Bug::Resolution::INVALID:
            return {i18nc("@info bug resolution", "Not a valid report/crash"), QString()};
        case Bugzilla::Bug::Resolution::UPSTREAM:
        case Bugzilla::Bug::Resolution::DOWNSTREAM:
            return {
                i18nc("@info bug resolution", "Not caused by a problem in the KDE's Applications or libraries"),
                i18nc("@info bug resolution", "the bug is caused by a problem in an external application or library, or by a distribution or packaging issue")};
        case Bugzilla::Bug::Resolution::WONTFIX:
        case Bugzilla::Bug::Resolution::LATER:
        case Bugzilla::Bug::Resolution::REMIND:
        case Bugzilla::Bug::Resolution::MOVED:
        case Bugzilla::Bug::Resolution::WAITINGFORINFO:
        case Bugzilla::Bug::Resolution::BACKTRACE:
        case Bugzilla::Bug::Resolution::UNMAINTAINED:
        case Bugzilla::Bug::Resolution::NONE:
            return {QVariant::fromValue(bug->resolution()).toString(), QString()};
        case Bugzilla::Bug::Resolution::Unknown:
            break;
        }
        return {};

    case Bugzilla::Bug::Status::NEEDSINFO:
        return {i18nc("@info bug status", "Temporarily closed, because of a lack of information"), QString()};

    case Bugzilla::Bug::Status::Unknown:
        break;
    }
    return {};
}

void BugzillaReportInformationDialog::bugFetchFinished(Bugzilla::Bug::Ptr bug, QObject *jobOwner)
{
    if (jobOwner != this || !isVisible()) {
        return;
    }

    if (!bug) {
        bugFetchError(i18nc("@info",
                            "Invalid report information (malformed data). This could "
                            "mean that the bug report does not exist, or the bug tracking site "
                            "is experiencing a problem."),
                      this);
        return;
    }

    Q_ASSERT(!m_bug); // m_bug must only be set once we've selected one!

    // Handle duplicate state
    if (bug->dupe_of() > 0) {
        ui.m_statusWidget->setIdle(QString());

        KGuiItem yesItem = KStandardGuiItem::yes();
        yesItem.setText(
            i18nc("@action:button let the user to choose to read the "
                  "main report",
                  "Yes, read the main report"));

        KGuiItem noItem = KStandardGuiItem::no();
        noItem.setText(
            i18nc("@action:button let the user choose to read the original "
                  "report",
                  "No, let me read the report I selected"));

        auto ret = KMessageBox::questionYesNo(this,
                                              xi18nc("@info",
                                                     "The report you selected (bug %1) is already "
                                                     "marked as duplicate of bug %2. "
                                                     "Do you want to read that report instead? (recommended)",
                                                     bug->id(),
                                                     QString::number(bug->dupe_of())),
                                              i18nc("@title:window", "Nested duplicate detected"),
                                              yesItem,
                                              noItem);
        if (ret == KMessageBox::Yes) {
            qDebug() << "REDIRECT";
            showBugReport(bug->dupe_of());
            return;
        }
    }

    // Process comments...
    m_bug = bug;
    m_parent->bugzillaManager()->fetchComments(m_bug, this);
}

void BugzillaReportInformationDialog::onCommentsFetched(QList<Bugzilla::Comment::Ptr> bugComments, QObject *jobOwner)
{
    if (jobOwner != this || !isVisible()) {
        return;
    }

    Q_ASSERT(m_bug);

    // Generate html for comments (with proper numbering)
    auto duplicatesMark = QLatin1String("has been marked as a duplicate of this bug.");

    // TODO: the way comment objects are turned into comment strings is fairly
    //    awkward and does not particularly object-centric. May benefit from a
    //    slight redesign.
    QString comments;
    QString description; // aka first comment
    if (bugComments.size() > 0) {
        description = bugComments.takeFirst()->text();
    }
    for (auto it = bugComments.constBegin(); it != bugComments.constEnd(); ++it) {
        QString comment = (*it)->text();
        // Don't add duplicates mark comments
        if (!comment.contains(duplicatesMark)) {
            comment.replace(QLatin1Char('\n'), QLatin1String("<br />"));
            const int i = it - bugComments.constBegin();
            comments +=
                i18nc("comment $number to use as subtitle", "<h4>Comment %1:</h4>", (i + 1)) + QStringLiteral("<p>") + comment + QStringLiteral("</p><hr />");
            // Count the inline attached crashes (DrKonqi feature)
            auto attachedCrashMark = QLatin1String("New crash information added by DrKonqi");
            if (comment.contains(attachedCrashMark)) {
                m_duplicatesCount++;
            }
        } else {
            // Count duplicate
            m_duplicatesCount++;
        }
    }

    // Generate a non-geek readable status
    auto str = statusString2(m_bug);
    QString customStatusString = str.statusString;
    m_closedStateString = str.closedStateString;

    // Generate notes
    QString notes = xi18n(
        "<p><note>The bug report's title is often written by its reporter "
        "and may not reflect the bug's nature, root cause or other visible "
        "symptoms you could use to compare to your crash. Please read the "
        "complete report and all the comments below.</note></p>");

    if (m_duplicatesCount >= 10) { // Consider a possible mass duplicate crash
        notes += xi18np(
            "<p><note>This bug report has %1 duplicate report. That means this "
            "is probably a <strong>common crash</strong>. <i>Please consider only "
            "adding a comment or a note if you can provide new valuable "
            "information which was not already mentioned.</i></note></p>",
            "<p><note>This bug report has %1 duplicate reports. That means this "
            "is probably a <strong>common crash</strong>. <i>Please consider only "
            "adding a comment or a note if you can provide new valuable "
            "information which was not already mentioned.</i></note></p>",
            m_duplicatesCount);
    }

    // Generate HTML text
    QString text = i18nc("@info bug report title (quoted)", "<h3>\"%1\"</h3>", m_bug->summary()) + notes
        + i18nc("@info bug report status", "<h4>Bug Report Status: %1</h4>", customStatusString)
        + i18nc("@info bug report product and component", "<h4>Affected Component: %1 (%2)</h4>", m_bug->product(), m_bug->component())
        + i18nc("@info bug report description", "<h3>Description of the bug</h3><p>%1</p>", description.replace(QLatin1Char('\n'), QLatin1String("<br />")));

    if (!comments.isEmpty()) {
        text += i18nc("@label:textbox bug report comments (already formatted)", "<h2>Additional Comments</h2>%1", comments);
    }

    ui.m_infoBrowser->setText(text);
    ui.m_infoBrowser->setEnabled(true);

    m_suggestButton->setEnabled(m_relatedButtonEnabled);
    m_suggestButton->setVisible(m_relatedButtonEnabled);

    ui.m_statusWidget->setIdle(xi18nc("@info:status", "Showing bug %1", QString::number(m_bug->id())));
}

void BugzillaReportInformationDialog::markAsDuplicate()
{
    Q_EMIT possibleDuplicateSelected(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::attachToBugReport()
{
    Q_EMIT attachToBugReportSelected(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::cancelAssistant()
{
    m_parent->assistant()->close();
    hide();
}

void BugzillaReportInformationDialog::relatedReportClicked()
{
    auto *confirmation = new BugzillaReportConfirmationDialog(m_bugNumber, (m_duplicatesCount >= 10), m_closedStateString, this);
    confirmation->show();
}

void BugzillaReportInformationDialog::bugFetchError(const QString &err, QObject *jobOwner)
{
    if (jobOwner == this && isVisible()) {
        KMessageBox::error(this,
                           xi18nc("@info/rich",
                                  "Error fetching the bug report<nl/>"
                                  "<message>%1.</message><nl/>"
                                  "Please wait some time and try again.",
                                  err));
        m_suggestButton->setEnabled(false);
        ui.m_infoBrowser->setText(i18nc("@info", "Error fetching the bug report"));
        ui.m_statusWidget->setIdle(i18nc("@info:status", "Error fetching the bug report"));
        ui.m_retryButton->setVisible(true);
    }
}

void BugzillaReportInformationDialog::toggleShowOwnBacktrace(bool show)
{
    QList<int> sizes;
    if (show) {
        int size = (ui.m_reportSplitter->sizeHint().width() - ui.m_reportSplitter->handleWidth()) / 2;
        sizes << size << size;
    } else {
        sizes << ui.m_reportSplitter->sizeHint().width() << 0; // Hide backtrace
    }
    ui.m_reportSplitter->setSizes(sizes);

    // Save the current show value
    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    config.writeEntry("ShowOwnBacktrace", show);
}
