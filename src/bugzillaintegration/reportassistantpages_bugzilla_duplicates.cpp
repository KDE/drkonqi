/*******************************************************************
 * reportassistantpages_bugzilla_duplicates.cpp
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "reportassistantpages_bugzilla_duplicates.h"

#include <QDebug>
#include <QHeaderView>
#include <QTimer>
#include <QTreeWidgetItem>

#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageBox>

#include "bugzillareportinformationdialog.h"
#include "drkonqi_globals.h"
#include "reportinterface.h"

BugzillaDuplicatesPage::BugzillaDuplicatesPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
{
    connect(bugzillaManager(), &BugzillaManager::searchFinished, this, &BugzillaDuplicatesPage::searchFinished);
    connect(bugzillaManager(), &BugzillaManager::searchError, this, &BugzillaDuplicatesPage::searchError);

    ui.setupUi(this);
    ui.information->hide();
    // clang-format off
    connect(ui.m_bugListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemClicked(QTreeWidgetItem*,int)));
    connect(ui.m_bugListWidget, &QTreeWidget::itemSelectionChanged, this, &BugzillaDuplicatesPage::itemSelectionChanged);
    // clang-format on

    QHeaderView *header = ui.m_bugListWidget->header();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Interactive);

    m_searchMoreGuiItem = KGuiItem2(i18nc("@action:button", "Search for more reports"),
                                    QIcon::fromTheme(QStringLiteral("edit-find")),
                                    i18nc("@info:tooltip",
                                          "Use this button to "
                                          "search for more similar bug reports"));
    KGuiItem::assign(ui.m_searchMoreButton, m_searchMoreGuiItem);
    connect(ui.m_searchMoreButton, &QAbstractButton::clicked, this, &BugzillaDuplicatesPage::searchMore);

    m_retrySearchGuiItem = KGuiItem2(i18nc("@action:button", "Retry search"),
                                     QIcon::fromTheme(QStringLiteral("edit-find")),
                                     i18nc("@info:tooltip",
                                           "Use this button to "
                                           "retry the search that previously "
                                           "failed."));

    KGuiItem::assign(ui.m_openReportButton,
                     KGuiItem2(i18nc("@action:button", "Open selected report"),
                               QIcon::fromTheme(QStringLiteral("document-preview")),
                               i18nc("@info:tooltip",
                                     "Use this button to view "
                                     "the information of the selected bug report.")));
    connect(ui.m_openReportButton, &QAbstractButton::clicked, this, &BugzillaDuplicatesPage::openSelectedReport);

    KGuiItem::assign(ui.m_stopSearchButton,
                     KGuiItem2(i18nc("@action:button", "Stop searching"),
                               QIcon::fromTheme(QStringLiteral("process-stop")),
                               i18nc("@info:tooltip",
                                     "Use this button to stop "
                                     "the current search.")));
    ui.m_stopSearchButton->setText(QString()); // FIXME
    connect(ui.m_stopSearchButton, &QAbstractButton::clicked, this, &BugzillaDuplicatesPage::stopCurrentSearch);

    // Possible duplicates list and buttons
    // clang-format off
    connect(ui.m_selectedDuplicatesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));
    // clang-format on
    connect(ui.m_selectedDuplicatesList, &QListWidget::itemSelectionChanged, this, &BugzillaDuplicatesPage::possibleDuplicateSelectionChanged);

    KGuiItem::assign(ui.m_removeSelectedDuplicateButton,
                     KGuiItem2(i18nc("@action:button remove the selected item from a list", "Remove"),
                               QIcon::fromTheme(QStringLiteral("list-remove")),
                               i18nc("@info:tooltip", "Use this button to remove a selected possible duplicate")));
    ui.m_removeSelectedDuplicateButton->setEnabled(false);
    connect(ui.m_removeSelectedDuplicateButton, &QAbstractButton::clicked, this, &BugzillaDuplicatesPage::removeSelectedDuplicate);

    ui.m_attachToReportIcon->setPixmap(QIcon::fromTheme(QStringLiteral("mail-attachment")).pixmap(16, 16));
    ui.m_attachToReportIcon->setFixedSize(16, 16);
    ui.m_attachToReportIcon->setVisible(false);
    ui.m_attachToReportLabel->setVisible(false);
    ui.m_attachToReportLabel->setContextMenuPolicy(Qt::NoContextMenu);
    connect(ui.m_attachToReportLabel, &QLabel::linkActivated, this, &BugzillaDuplicatesPage::cancelAttachToBugReport);
    ui.information->setContextMenuPolicy(Qt::NoContextMenu);
    connect(ui.information, &QLabel::linkActivated, this, &BugzillaDuplicatesPage::informationClicked);
    showDuplicatesPanel(false);
}

void BugzillaDuplicatesPage::aboutToShow()
{
    // Perform initial search if we are not currently searching and if there are no results yet
    if (!m_searching && ui.m_bugListWidget->topLevelItemCount() <= 0 && canSearchMore()) {
        searchMore();
    }
}

void BugzillaDuplicatesPage::aboutToHide()
{
    stopCurrentSearch();

    // Save selected possible duplicates by user
    QStringList possibleDuplicates;
    int count = ui.m_selectedDuplicatesList->count();
    for (int i = 0; i < count; i++) {
        possibleDuplicates << ui.m_selectedDuplicatesList->item(i)->text();
    }
    reportInterface()->setPossibleDuplicates(possibleDuplicates);

    // Save possible duplicates by query
    QStringList duplicatesByQuery;
    count = ui.m_bugListWidget->topLevelItemCount();
    for (int i = 0; i < count; i++) {
        duplicatesByQuery << ui.m_bugListWidget->topLevelItem(i)->text(0);
    }
    reportInterface()->setPossibleDuplicatesByQuery(duplicatesByQuery);
}

bool BugzillaDuplicatesPage::isComplete()
{
    return !m_searching;
}

bool BugzillaDuplicatesPage::showNextPage()
{
    // Ask the user to check all the possible duplicates...
    if (ui.m_bugListWidget->topLevelItemCount() > 0 && ui.m_selectedDuplicatesList->count() == 0 && reportInterface()->attachToBugNumber() == 0
        && !m_foundDuplicate) {
        // The user didn't selected any possible duplicate nor a report to attach the new info.
        // Double check this, we need to reduce the duplicate count.
        KGuiItem noDuplicatesButton;
        noDuplicatesButton.setText(i18n("There are no real duplicates"));
        noDuplicatesButton.setWhatsThis(
            i18n("Press this button to declare that, in your opinion "
                 "and according to your experience, the reports found "
                 "as similar do not match the crash you have "
                 "experienced, and you believe it is unlikely that a "
                 "better match would be found after further review."));
        noDuplicatesButton.setIcon(QIcon::fromTheme(QStringLiteral("dialog-cancel")));

        KGuiItem letMeCheckMoreReportsButton;
        letMeCheckMoreReportsButton.setText(i18n("Let me check more reports"));
        letMeCheckMoreReportsButton.setWhatsThis(
            i18n("Press this button if you would rather "
                 "review more reports in order to find a "
                 "match for the crash you have experienced."));
        letMeCheckMoreReportsButton.setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));

        if (KMessageBox::questionYesNo(this,
                                       i18nc("@info",
                                             "You have not selected any possible duplicates, or a report to which to attach your "
                                             "crash information. Have you read all the reports, and can you confirm that there are no "
                                             "real duplicates?"),
                                       i18nc("@title:window", "No selected possible duplicates"),
                                       letMeCheckMoreReportsButton,
                                       noDuplicatesButton)
            == KMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

void BugzillaDuplicatesPage::searchMore()
{
    if (m_offset < 0) {
        m_offset = 0; // initialize, -1 means no search done yet
    }

    // This is fairly inefficient, unfortunately the APIs offset/limit system
    // is not useful to us. The search is always sorting by lowest id, and
    // negative offsets are not a thing. So, offset=0&limit=1 gives the first
    // ever reported bug in the product, while what we want is the latest.
    // We also cannot query all perintent bug ids by default. While the API
    // is reasonably fast, it'll still produce upwards of 2MiB just for the
    // ids of a dolphin crash (as it includes all sorts of extra products).
    // So we are left with somewhat shoddy time-based queries.

    markAsSearching(true);
    ui.m_statusWidget->setBusy(i18nc("@info:status", "Searching for duplicates..."));

    // Grab the default severity for newbugs
    static QString severity = reportInterface()->newBugReportTemplate().severity;

    bugzillaManager()->searchBugs(reportInterface()->relatedBugzillaProducts(),
                                  severity,
                                  reportInterface()->firstBacktraceFunctions().join(QLatin1Char(' ')),
                                  m_offset);
}

void BugzillaDuplicatesPage::stopCurrentSearch()
{
    if (m_searching) {
        bugzillaManager()->stopCurrentSearch();

        markAsSearching(false);

        if (m_offset < 0) { // Never searched
            ui.m_statusWidget->setIdle(i18nc("@info:status", "Search stopped."));
        } else {
            ui.m_statusWidget->setIdle(i18nc("@info:status", "Search stopped. Showing results."));
        }
    }
}

void BugzillaDuplicatesPage::markAsSearching(bool searching)
{
    m_searching = searching;
    emitCompleteChanged();

    ui.m_bugListWidget->setEnabled(!searching);
    ui.m_searchMoreButton->setEnabled(!searching);
    ui.m_searchMoreButton->setVisible(!searching);
    ui.m_stopSearchButton->setEnabled(searching);
    ui.m_stopSearchButton->setVisible(searching);

    ui.m_selectedDuplicatesList->setEnabled(!searching);
    ui.m_selectedPossibleDuplicatesLabel->setEnabled(!searching);
    ui.m_removeSelectedDuplicateButton->setEnabled(!searching && !ui.m_selectedDuplicatesList->selectedItems().isEmpty());

    ui.m_attachToReportLabel->setEnabled(!searching);

    if (!searching) {
        itemSelectionChanged();
    } else {
        ui.m_openReportButton->setEnabled(false);
    }
}

bool BugzillaDuplicatesPage::canSearchMore()
{
    return !m_atEnd;
}

static QString statusString(const Bugzilla::Bug::Ptr &bug)
{
    // Generate a non-geek readable status
    switch (bug->status()) {
    case Bugzilla::Bug::Status::UNCONFIRMED:
    case Bugzilla::Bug::Status::CONFIRMED:
    case Bugzilla::Bug::Status::ASSIGNED:
    case Bugzilla::Bug::Status::REOPENED:
        return i18nc("@info bug status", "[Open]");

    case Bugzilla::Bug::Status::RESOLVED:
    case Bugzilla::Bug::Status::VERIFIED:
    case Bugzilla::Bug::Status::CLOSED:
        switch (bug->resolution()) {
        case Bugzilla::Bug::Resolution::FIXED:
            return i18nc("@info bug resolution", "[Fixed]");
        case Bugzilla::Bug::Resolution::WORKSFORME:
            return i18nc("@info bug resolution", "[Non-reproducible]");
        case Bugzilla::Bug::Resolution::DUPLICATE:
            return i18nc("@info bug resolution", "[Duplicate report]");
        case Bugzilla::Bug::Resolution::INVALID:
            return i18nc("@info bug resolution", "[Invalid]");
        case Bugzilla::Bug::Resolution::UPSTREAM:
        case Bugzilla::Bug::Resolution::DOWNSTREAM:
            return i18nc("@info bug resolution", "[External problem]");
        case Bugzilla::Bug::Resolution::WONTFIX:
        case Bugzilla::Bug::Resolution::LATER:
        case Bugzilla::Bug::Resolution::REMIND:
        case Bugzilla::Bug::Resolution::MOVED:
        case Bugzilla::Bug::Resolution::WAITINGFORINFO:
        case Bugzilla::Bug::Resolution::BACKTRACE:
        case Bugzilla::Bug::Resolution::UNMAINTAINED:
        case Bugzilla::Bug::Resolution::NONE:
            return QStringLiteral("[%1]").arg(QVariant::fromValue(bug->resolution()).toString());
        case Bugzilla::Bug::Resolution::Unknown:
            break;
        }
        break;
    case Bugzilla::Bug::Status::NEEDSINFO:
        return i18nc("@info bug status", "[Incomplete]");

    case Bugzilla::Bug::Status::Unknown:
        break;
    }
    return QString();
}

void BugzillaDuplicatesPage::searchFinished(const QList<Bugzilla::Bug::Ptr> &list)
{
    KGuiItem::assign(ui.m_searchMoreButton, m_searchMoreGuiItem);

    int results = list.count();
    m_offset += results;
    if (results > 0) {
        m_atEnd = false;

        markAsSearching(false);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "Showing results."));

        for (int i = 0; i < results; i++) {
            Bugzilla::Bug::Ptr bug = list.at(i);

            QString title = statusString(bug) + QLatin1Char(' ') + bug->summary();

            QStringList fields = QStringList() << QString::number(bug->id()) << title;

            auto *item = new QTreeWidgetItem(fields);
            item->setToolTip(0, bug->summary());
            item->setToolTip(1, bug->summary());

            ui.m_bugListWidget->addTopLevelItem(item);
        }

        if (!m_foundDuplicate) {
            markAsSearching(true);
            auto *job = new DuplicateFinderJob(list, bugzillaManager(), this);
            connect(job, &KJob::result, this, &BugzillaDuplicatesPage::analyzedDuplicates);
            job->start();
        }

        ui.m_bugListWidget->sortItems(0, Qt::DescendingOrder);
        ui.m_bugListWidget->resizeColumnToContents(1);

        if (!canSearchMore()) {
            ui.m_searchMoreButton->setEnabled(false);
        }

    } else {
        m_atEnd = true;

        if (canSearchMore()) {
            // We don't call markAsSearching(false) to avoid flicker
            // Delayed call to searchMore to avoid unexpected behaviour (signal/slot)
            // because we are in a slot, and searchMore() will be ending calling this slot again
            QTimer::singleShot(0, this, &BugzillaDuplicatesPage::searchMore);
        } else {
            markAsSearching(false);
            ui.m_statusWidget->setIdle(i18nc("@info:status",
                                             "Search Finished. "
                                             "No reports found."));
            ui.m_searchMoreButton->setEnabled(false);
            if (ui.m_bugListWidget->topLevelItemCount() == 0) {
                // No reports to mark as possible duplicate
                ui.m_selectedDuplicatesList->setEnabled(false);
            }
        }
    }
}

static bool isStatusOpen(Bugzilla::Bug::Status status)
{
    switch (status) {
    case Bugzilla::Bug::Status::UNCONFIRMED:
    case Bugzilla::Bug::Status::CONFIRMED:
    case Bugzilla::Bug::Status::ASSIGNED:
    case Bugzilla::Bug::Status::REOPENED:
        return true;
    case Bugzilla::Bug::Status::RESOLVED:
    case Bugzilla::Bug::Status::NEEDSINFO:
    case Bugzilla::Bug::Status::VERIFIED:
    case Bugzilla::Bug::Status::CLOSED:
        return false;

    case Bugzilla::Bug::Status::Unknown:
        break;
    }
    return false;
}

static bool isStatusClosed(Bugzilla::Bug::Status status)
{
    return !isStatusOpen(status);
}

void BugzillaDuplicatesPage::analyzedDuplicates(KJob *j)
{
    markAsSearching(false);

    auto *job = static_cast<DuplicateFinderJob *>(j);
    m_result = job->result();
    m_foundDuplicate = m_result.parentDuplicate;
    reportInterface()->setDuplicateId(m_result.parentDuplicate);
    ui.m_searchMoreButton->setEnabled(!m_foundDuplicate);
    ui.information->setVisible(m_foundDuplicate);
    auto status = m_result.status;
    const int duplicate = m_result.duplicate;
    const int parentDuplicate = m_result.parentDuplicate;

    if (m_foundDuplicate) {
        const QList<QTreeWidgetItem *> items = ui.m_bugListWidget->findItems(QString::number(parentDuplicate), Qt::MatchExactly, 0);
        const QBrush brush = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NeutralBackground);
        for (QTreeWidgetItem *item : std::as_const(items)) {
            for (int i = 0; i < item->columnCount(); ++i) {
                item->setBackground(i, brush);
            }
        }

        QString text;
        if (isStatusOpen(status) || status == Bugzilla::Bug::Status::NEEDSINFO) {
            text =
                (parentDuplicate == duplicate ? i18nc("@label",
                                                      "Your crash is a <strong>duplicate</strong> and has already been reported as <a href=\"%1\">Bug %1</a>.",
                                                      QString::number(duplicate))
                                              : i18nc("@label",
                                                      "Your crash has already been reported as <a href=\"%1\">Bug %1</a>, which is a "
                                                      "<strong>duplicate</strong> of <a href=\"%2\">Bug %2</a>",
                                                      QString::number(duplicate),
                                                      QString::number(parentDuplicate)))
                + QLatin1Char('\n')
                + i18nc("@label",
                        "Only <strong><a href=\"%1\">attach</a></strong> if you can add needed information to the bug report.",
                        QStringLiteral("attach"));
        } else if (isStatusClosed(status)) {
            text = (parentDuplicate == duplicate
                        ? i18nc("@label",
                                "Your crash has already been reported as <a href=\"%1\">Bug %1</a> which has been <strong>closed</strong>.",
                                QString::number(duplicate))
                        : i18nc("@label",
                                "Your crash has already been reported as <a href=\"%1\">Bug %1</a>, which is a duplicate of the <strong>closed</strong> <a "
                                "href=\"%2\">Bug %2</a>.",
                                QString::number(duplicate),
                                QString::number(parentDuplicate)));
        }
        ui.information->setText(text);
    }
}

void BugzillaDuplicatesPage::informationClicked(const QString &activatedLink)
{
    if (activatedLink == QLatin1String("attach")) {
        attachToBugReport(m_result.parentDuplicate);
    } else {
        int number = activatedLink.toInt();
        if (number) {
            showReportInformationDialog(number, false);
        }
    }
}

void BugzillaDuplicatesPage::searchError(QString err)
{
    KGuiItem::assign(ui.m_searchMoreButton, m_retrySearchGuiItem);
    markAsSearching(false);

    ui.m_statusWidget->setIdle(i18nc("@info:status", "Error fetching the bug report list"));

    KMessageBox::error(this,
                       xi18nc("@info/rich",
                              "Error fetching the bug report list<nl/>"
                              "<message>%1.</message><nl/>"
                              "Please wait some time and try again.",
                              err));
}

void BugzillaDuplicatesPage::openSelectedReport()
{
    QList<QTreeWidgetItem *> selected = ui.m_bugListWidget->selectedItems();
    if (selected.count() == 1) {
        itemClicked(selected.at(0), 0);
    }
}

void BugzillaDuplicatesPage::itemClicked(QTreeWidgetItem *item, int col)
{
    Q_UNUSED(col);
    showReportInformationDialog(item->text(0).toInt());
}

void BugzillaDuplicatesPage::itemClicked(QListWidgetItem *item)
{
    showReportInformationDialog(item->text().toInt());
}

void BugzillaDuplicatesPage::showReportInformationDialog(int bugNumber, bool relatedButtonEnabled)
{
    if (bugNumber <= 0) {
        return;
    }

    auto *infoDialog = new BugzillaReportInformationDialog(this);
    connect(infoDialog, &BugzillaReportInformationDialog::possibleDuplicateSelected, this, &BugzillaDuplicatesPage::addPossibleDuplicateNumber);
    connect(infoDialog, &BugzillaReportInformationDialog::attachToBugReportSelected, this, &BugzillaDuplicatesPage::attachToBugReport);

    infoDialog->showBugReport(bugNumber, relatedButtonEnabled);
}

void BugzillaDuplicatesPage::itemSelectionChanged()
{
    ui.m_openReportButton->setEnabled(ui.m_bugListWidget->selectedItems().count() == 1);
}

void BugzillaDuplicatesPage::addPossibleDuplicateNumber(int bugNumber)
{
    QString stringNumber = QString::number(bugNumber);
    if (ui.m_selectedDuplicatesList->findItems(stringNumber, Qt::MatchExactly).isEmpty()) {
        ui.m_selectedDuplicatesList->addItem(stringNumber);
    }

    showDuplicatesPanel(true);
}

void BugzillaDuplicatesPage::removeSelectedDuplicate()
{
    QList<QListWidgetItem *> items = ui.m_selectedDuplicatesList->selectedItems();
    if (items.length() > 0) {
        delete ui.m_selectedDuplicatesList->takeItem(ui.m_selectedDuplicatesList->row(items.at(0)));
    }

    if (ui.m_selectedDuplicatesList->count() == 0) {
        showDuplicatesPanel(false);
    }
}

void BugzillaDuplicatesPage::showDuplicatesPanel(bool show)
{
    ui.m_removeSelectedDuplicateButton->setVisible(show);
    ui.m_selectedDuplicatesList->setVisible(show);
    ui.m_selectedPossibleDuplicatesLabel->setVisible(show);
}

void BugzillaDuplicatesPage::possibleDuplicateSelectionChanged()
{
    ui.m_removeSelectedDuplicateButton->setEnabled(!ui.m_selectedDuplicatesList->selectedItems().isEmpty());
}

void BugzillaDuplicatesPage::attachToBugReport(int bugNumber)
{
    ui.m_attachToReportLabel->setText(xi18nc("@label",
                                             "The report is going to be "
                                             "<strong>attached</strong> to bug %1. "
                                             "<a href=\"#\">Cancel</a>",
                                             QString::number(bugNumber)));
    ui.m_attachToReportLabel->setVisible(true);
    ui.m_attachToReportIcon->setVisible(true);
    reportInterface()->setAttachToBugNumber(bugNumber);
}

void BugzillaDuplicatesPage::cancelAttachToBugReport()
{
    ui.m_attachToReportLabel->setVisible(false);
    ui.m_attachToReportIcon->setVisible(false);
    reportInterface()->setAttachToBugNumber(0);
}
