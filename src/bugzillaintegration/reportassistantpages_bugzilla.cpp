/*******************************************************************
 * reportassistantpages_bugzilla.cpp
 * SPDX-FileCopyrightText: 2009, 2010, 2011 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "reportassistantpages_bugzilla.h"

#include <chrono>

#include <QAction>
#include <QCheckBox>
#include <QCursor>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLabel>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QTimer>
#include <QToolTip>
#include <QWindow>

#include "drkonqi_debug.h"
#include <KCapacityBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <kcompletion_version.h>
#include <kwallet.h>

/* Unhandled error dialog includes */
#include <KConfigGroup>
#include <KIO/Job>
#include <KJobWidgets>
#include <KSharedConfig>

#include "applicationdetailsexamples.h"
#include "bugzillalib.h"
#include "crashedapplication.h"
#include "drkonqi.h"
#include "drkonqi_globals.h"
#include "reportinformationdialog.h"
#include "reportinterface.h"
#include "statuswidget.h"
#include "systeminformation.h"

using namespace std::chrono_literals;

static const char kWalletEntryUsername[] = "username";
static const char kWalletEntryPassword[] = "password";

static QString konquerorKWalletEntryName = KDE_BUGZILLA_URL + QStringLiteral("index.cgi#login");
static const char konquerorKWalletEntryUsername[] = "Bugzilla_login";
static const char konquerorKWalletEntryPassword[] = "Bugzilla_password";

// BEGIN BugzillaLoginPage

BugzillaLoginPage::BugzillaLoginPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
    , m_walletEntryName(DrKonqi::isTestingBugzilla() ? QStringLiteral("drkonqi_bugzilla_test_mode") : QStringLiteral("drkonqi_bugzilla"))
    , m_wallet(nullptr)
    , m_walletWasOpenedBefore(false)
{
    connect(bugzillaManager(), &BugzillaManager::loginFinished, this, &BugzillaLoginPage::loginFinished);
    connect(bugzillaManager(), &BugzillaManager::loginError, this, &BugzillaLoginPage::loginError);

    ui.setupUi(this);
    ui.m_statusWidget->setIdle(i18nc("@info:status '1' is replaced with the short URL of the bugzilla ",
                                     "You need to login with your %1 account in order to proceed.",
                                     QLatin1String(KDE_BUGZILLA_SHORT_URL)));

    KGuiItem::assign(ui.m_loginButton,
                     KGuiItem2(i18nc("@action:button", "Login"),
                               QIcon::fromTheme(QStringLiteral("network-connect")),
                               i18nc("@info:tooltip",
                                     "Use this button to login "
                                     "to the KDE bug tracking system using the provided "
                                     "e-mail address and password.")));
    ui.m_loginButton->setEnabled(false);

    connect(ui.m_loginButton, &QPushButton::clicked, this, &BugzillaLoginPage::loginClicked);

#if KCOMPLETION_VERSION >= QT_VERSION_CHECK(5, 81, 0)
    connect(ui.m_userEdit, &KLineEdit::returnKeyPressed, this, &BugzillaLoginPage::loginClicked);
#else
    connect(ui.m_userEdit, &KLineEdit::returnPressed, this, &BugzillaLoginPage::loginClicked);
#endif
    connect(ui.m_passwordEdit->lineEdit(), &QLineEdit::returnPressed, this, &BugzillaLoginPage::loginClicked);

    connect(ui.m_userEdit, &KLineEdit::textChanged, this, &BugzillaLoginPage::updateLoginButtonStatus);
    connect(ui.m_passwordEdit, &KPasswordLineEdit::passwordChanged, this, &BugzillaLoginPage::updateLoginButtonStatus);

    ui.m_noticeLabel->setText(xi18nc("@info/rich",
                                     "<note>You need a user account on the "
                                     "<link url='%1'>KDE bug tracking system</link> in order to "
                                     "file a bug report, because we may need to contact you later "
                                     "for requesting further information. If you do not have "
                                     "one, you can freely <link url='%2'>create one here</link>. "
                                     "Please do not use disposable email accounts.</note>",
                                     DrKonqi::crashedApplication()->bugReportAddress(),
                                     KDE_BUGZILLA_CREATE_ACCOUNT_URL));

    // Don't advertise saving credentials when we can't save them.
    // https://bugs.kde.org/show_bug.cgi?id=363570
    if (!KWallet::Wallet::isEnabled()) {
        ui.m_savePasswordCheckBox->setVisible(false);
        ui.m_savePasswordCheckBox->setCheckState(Qt::Unchecked);
    }
}

bool BugzillaLoginPage::isComplete()
{
    return bugzillaManager()->getLogged();
}

void BugzillaLoginPage::updateLoginButtonStatus()
{
    ui.m_loginButton->setEnabled(canLogin());
}

void BugzillaLoginPage::loginError(const QString &error)
{
    loginFinished(false);
    ui.m_statusWidget->setIdle(xi18nc("@info:status",
                                      "Error when trying to login: "
                                      "<message>%1</message>",
                                      error));
}

void BugzillaLoginPage::aboutToShow()
{
    if (bugzillaManager()->getLogged()) {
        ui.m_loginButton->setEnabled(false);

        ui.m_userEdit->setEnabled(false);
        ui.m_userEdit->clear();
        ui.m_passwordEdit->setEnabled(false);
        ui.m_passwordEdit->clear();

        ui.m_loginButton->setVisible(false);
        ui.m_userEdit->setVisible(false);
        ui.m_passwordEdit->setVisible(false);
        ui.m_userLabel->setVisible(false);
        ui.m_passwordLabel->setVisible(false);

        ui.m_savePasswordCheckBox->setVisible(false);

        ui.m_noticeLabel->setVisible(false);

        ui.m_statusWidget->setIdle(
            i18nc("@info:status the user is logged at the bugtracker site "
                  "as USERNAME",
                  "Logged in at the KDE bug tracking system (%1) as: %2.",
                  QLatin1String(KDE_BUGZILLA_SHORT_URL),
                  bugzillaManager()->getUsername()));
    } else {
        // Try to show wallet dialog once this dialog is shown
        QTimer::singleShot(100ms, this, &BugzillaLoginPage::walletLogin);
        // ...also reset focus. walletLogin may be fully no-op and not set focus
        // if it failed to do anything which leaves the focus elsewhere
        ui.m_userEdit->setFocus(Qt::OtherFocusReason);
    }
}

bool BugzillaLoginPage::kWalletEntryExists(const QString &entryName)
{
    return !KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(), KWallet::Wallet::FormDataFolder(), entryName);
}

void BugzillaLoginPage::openWallet()
{
    // Store if the wallet was previously opened so we can know if we should close it later
    m_walletWasOpenedBefore = KWallet::Wallet::isOpen(KWallet::Wallet::NetworkWallet());

    // Request open the wallet
    WId windowId = 0;
    const auto *widget = qobject_cast<QWidget *>(this->parent());
    QWindow *window = widget->windowHandle();
    if (window) {
        windowId = window->winId();
    }

    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), windowId);
}

void BugzillaLoginPage::walletLogin()
{
    if (!m_wallet) {
        if (kWalletEntryExists(m_walletEntryName)) { // Key exists!
            openWallet();
            ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);
            // Was the wallet opened?
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                // Use wallet data to try login
                QMap<QString, QString> values;
                m_wallet->readMap(m_walletEntryName, values);
                QString username = values.value(QLatin1String(kWalletEntryUsername));
                QString password = values.value(QLatin1String(kWalletEntryPassword));

                if (!username.isEmpty() && !password.isEmpty()) {
                    ui.m_userEdit->setText(username);
                    ui.m_passwordEdit->setPassword(password);
                }
            }
        } else if (kWalletEntryExists(konquerorKWalletEntryName)) {
            // If the DrKonqi entry is empty, but a Konqueror entry exists, use and copy it.
            openWallet();
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                // Fetch Konqueror data
                QMap<QString, QString> values;
                m_wallet->readMap(konquerorKWalletEntryName, values);
                QString username = values.value(QLatin1String(konquerorKWalletEntryUsername));
                QString password = values.value(QLatin1String(konquerorKWalletEntryPassword));

                if (!username.isEmpty() && !password.isEmpty()) {
                    // Copy to DrKonqi own entries
                    values.clear();
                    values.insert(QLatin1String(kWalletEntryUsername), username);
                    values.insert(QLatin1String(kWalletEntryPassword), password);
                    m_wallet->writeMap(m_walletEntryName, values);

                    ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);

                    ui.m_userEdit->setText(username);
                    ui.m_passwordEdit->setPassword(password);
                }
            }
        }

        if (canLogin()) {
            loginClicked();
        }
    }
}

void BugzillaLoginPage::loginClicked()
{
    if (!canLogin()) {
        loginFinished(false);
        return;
    }

    updateWidget(false);

    if (ui.m_savePasswordCheckBox->checkState() == Qt::Checked) { // Wants to save data
        if (!m_wallet) {
            openWallet();
        }
        // Got wallet open ?
        if (m_wallet) {
            m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

            QMap<QString, QString> values;
            values.insert(QLatin1String(kWalletEntryUsername), ui.m_userEdit->text());
            values.insert(QLatin1String(kWalletEntryPassword), ui.m_passwordEdit->password());
            m_wallet->writeMap(m_walletEntryName, values);
        }
    } else { // User doesn't want to save or wants to remove.
        if (kWalletEntryExists(m_walletEntryName)) {
            if (!m_wallet) {
                openWallet();
            }
            // Got wallet open ?
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());
                m_wallet->removeEntry(m_walletEntryName);
            }
        }
    }

    login();
}

bool BugzillaLoginPage::canLogin() const
{
    return (!(ui.m_userEdit->text().isEmpty() || ui.m_passwordEdit->password().isEmpty()));
}

void BugzillaLoginPage::login()
{
    Q_ASSERT(canLogin());

    ui.m_statusWidget->setBusy(i18nc("@info:status '1' is a url, '2' the e-mail address",
                                     "Performing login at %1 as %2...",
                                     QLatin1String(KDE_BUGZILLA_SHORT_URL),
                                     ui.m_userEdit->text()));

    bugzillaManager()->tryLogin(ui.m_userEdit->text(), ui.m_passwordEdit->password());
}

void BugzillaLoginPage::updateWidget(bool enabled)
{
    ui.m_loginButton->setEnabled(enabled);

    ui.m_userLabel->setEnabled(enabled);
    ui.m_passwordLabel->setEnabled(enabled);

    ui.m_userEdit->setEnabled(enabled);
    ui.m_passwordEdit->setEnabled(enabled);
    ui.m_savePasswordCheckBox->setEnabled(enabled);
}

void BugzillaLoginPage::loginFinished(bool logged)
{
    if (logged) {
        emitCompleteChanged();

        aboutToShow();
        if (m_wallet) {
            if (m_wallet->isOpen() && !m_walletWasOpenedBefore) {
                m_wallet->lockWallet();
            }
        }

        Q_EMIT loggedTurnToNextPage();
    } else {
        ui.m_statusWidget->setIdle(i18nc("@info:status", "<b>Error: Invalid e-mail address or password</b>"));
        updateWidget(true);
        ui.m_userEdit->setFocus(Qt::OtherFocusReason);
    }
}

BugzillaLoginPage::~BugzillaLoginPage()
{
    // Close wallet if we close the assistant in this step
    if (m_wallet) {
        if (m_wallet->isOpen() && !m_walletWasOpenedBefore) {
            m_wallet->lockWallet();
        }
        delete m_wallet;
    }
}

// END BugzillaLoginPage

// BEGIN BugzillaInformationPage

BugzillaInformationPage::BugzillaInformationPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
    , m_textsOK(false)
    , m_requiredCharacters(1)
{
    ui.setupUi(this);

    m_textCompleteBar = new KCapacityBar(KCapacityBar::DrawTextInline, this);
    ui.horizontalLayout_2->addWidget(m_textCompleteBar);

    connect(ui.m_titleEdit, &KLineEdit::textChanged, this, &BugzillaInformationPage::checkTexts);
    connect(ui.m_detailsEdit, &QTextEdit::textChanged, this, &BugzillaInformationPage::checkTexts);

    connect(ui.m_titleLabel, &QLabel::linkActivated, this, &BugzillaInformationPage::showTitleExamples);
    connect(ui.m_detailsLabel, &QLabel::linkActivated, this, &BugzillaInformationPage::showDescriptionHelpExamples);

    ui.m_compiledSourcesCheckBox->setChecked(DrKonqi::systemInformation()->compiledSources());

    ui.m_messageWidget->setVisible(false);
    auto retryAction =
        new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18nc("@action/button retry button in error widget", "Retry"), ui.m_messageWidget);
    connect(retryAction, &QAction::triggered, this, [this] {
        ui.m_messageWidget->animatedHide();
        loadDistroCombo();
    });
    ui.m_messageWidget->addAction(retryAction);

    loadDistroCombo();
}

// Helps to track textual blocks we insert into text edits.
// This is intended to ensure the cursor is at the end of the first block
// e.g.
//   foo:\n
//    <- cursor here
//   bar:\n
// To that end the function accepts an input position which is -1 when not initialized.
// The first block initializes it to the position after appending, all subsquent blocks
// return the input position again. Blocks prepend themselves when the position is >=0
// Nice to have and easy to do but not strictly necessary -> https://bugs.kde.org/show_bug.cgi?id=438736
static int appendAndKeepPosition(QTextEdit *edit, const QString &text, int position)
{
    const bool hasPosition = position >= 0;

    if (hasPosition) {
        edit->append(QString()); // append empty paragraph in case there was previous block (i.e. add extra newline)
    }

    edit->append(text); // modifies cursor

    if (!hasPosition) {
        return edit->textCursor().position();
    }

    return position;
}

void BugzillaInformationPage::aboutToShow()
{
    // Calculate the minimum number of characters required for a description
    // If creating a new report: minimum 40, maximum 80
    // If attaching to an existent report: minimum 30, maximum 50
    int multiplier = (reportInterface()->attachToBugNumber() == 0) ? 10 : 5;
    m_requiredCharacters = 20 + (reportInterface()->selectedOptionsRating() * multiplier);

    // Fill the description textedit with some headings:
    if (ui.m_detailsEdit->toPlainText().isEmpty()) {
        int position = -1;
        if (reportInterface()->userCanProvideActionsAppDesktop()) {
            position = appendAndKeepPosition(ui.m_detailsEdit, QStringLiteral("- What I was doing when the application crashed:\n"), position);
        }
        if (reportInterface()->userCanProvideUnusualBehavior()) {
            position = appendAndKeepPosition(ui.m_detailsEdit, QStringLiteral("- Unusual behavior I noticed:\n"), position);
        }
        if (reportInterface()->userCanProvideApplicationConfigDetails()) {
            position = appendAndKeepPosition(ui.m_detailsEdit, QStringLiteral("- Custom settings of the application:\n"), position);
        }
        if (position >= 0) {
            QTextCursor cursor = ui.m_detailsEdit->textCursor();
            cursor.setPosition(position);
            ui.m_detailsEdit->setTextCursor(cursor);
        }
    }
    // If attaching this report to an existing one then the title is not needed
    bool showTitle = (reportInterface()->attachToBugNumber() == 0);
    ui.m_titleEdit->setVisible(showTitle);
    ui.m_titleLabel->setVisible(showTitle);

    // Force focus on the first input field for ease of use.
    // https://bugs.kde.org/show_bug.cgi?id=428350
    if (showTitle) {
        ui.m_titleEdit->setFocus();
    } else {
        ui.m_detailsEdit->setFocus();
    }

    checkTexts(); // May be the options (canDetail) changed and we need to recheck
}

int BugzillaInformationPage::currentDescriptionCharactersCount()
{
    QString description = ui.m_detailsEdit->toPlainText();

    // Do not count template messages, and other misc chars
    description.remove(QStringLiteral("What I was doing when the application crashed"));
    description.remove(QStringLiteral("Unusual behavior I noticed"));
    description.remove(QStringLiteral("Custom settings of the application"));
    description.remove(QLatin1Char('\n'));
    description.remove(QLatin1Char('-'));
    description.remove(QLatin1Char(':'));
    description.remove(QLatin1Char(' '));

    return description.size();
}

void BugzillaInformationPage::checkTexts()
{
    bool ok = !((ui.m_titleEdit->isVisible() && ui.m_titleEdit->text().isEmpty()) || ui.m_detailsEdit->toPlainText().isEmpty());

    QString message;
    int percent = currentDescriptionCharactersCount() * 100 / m_requiredCharacters;
    if (percent >= 100) {
        percent = 100;
        message = i18nc("the minimum required length of a text was reached", "Minimum length reached");
    } else {
        message = i18nc("the minimum required length of a text wasn't reached yet", "Provide more information");
    }
    m_textCompleteBar->setValue(percent);
    m_textCompleteBar->setText(message);

    if (ok != m_textsOK) {
        m_textsOK = ok;
        emitCompleteChanged();
    }
}

void BugzillaInformationPage::loadDistroCombo()
{
    // Alway have at least unspecified otherwise bugzilla would get to decide
    // the platform and that would likely be index=0 which is compiledfromsource
    ui.m_distroChooserCombo->addItem(QStringLiteral("unspecified"));

    Bugzilla::BugFieldClient client;
    auto job = client.getField(QStringLiteral("rep_platform"));
    connect(job, &KJob::finished, this, [this, client](KJob *job) {
        try {
            Bugzilla::BugField::Ptr field = client.getField(job);
            if (!field) {
                // This is a bit flimsy but only acts as save guard.
                // Ideally this code path is never hit.
                throw Bugzilla::RuntimeException(i18nc("@info/status error", "Failed to get platform list"));
            }
            setDistros(field);
        } catch (Bugzilla::Exception &e) {
            qCWarning(DRKONQI_LOG) << e.whatString();
            setDistroComboError(e.whatString());
        }
    });
}

void BugzillaInformationPage::setDistros(const Bugzilla::BugField::Ptr &field)
{
    ui.m_distroChooserCombo->clear();
    const QList<Bugzilla::BugFieldValue *> distros = field->values();
    for (const auto &distro : distros) {
        ui.m_distroChooserCombo->addItem(distro->name());
    }

    int index = 0;
    bool autoDetected = false;

    const QString detectedPlatform = DrKonqi::systemInformation()->bugzillaPlatform();
    if (detectedPlatform != QLatin1String("unspecified")) {
        index = ui.m_distroChooserCombo->findText(detectedPlatform);
        if (index >= 0) {
            autoDetected = true;
        }
    } else {
        // Restore previously selected bugzilla platform (distribution)
        KConfigGroup config(KSharedConfig::openConfig(), "BugzillaInformationPage");
        const QString entry = config.readEntry("BugzillaPlatform", "unspecified");
        index = ui.m_distroChooserCombo->findText(entry);
    }

    if (index < 0) { // failed to restore value
        index = ui.m_distroChooserCombo->findText(QStringLiteral("unspecified"));
    }
    if (index < 0) { // also failed to find unspecified... shouldn't happen
        index = 0;
    }

    ui.m_distroChooserCombo->setCurrentIndex(index);
    ui.m_distroChooserCombo->setVisible(autoDetected);
    ui.m_distroChooserCombo->setDisabled(false);
}

void BugzillaInformationPage::setDistroComboError(const QString &error)
{
    // NB: not being able to resolve the platform isn't a blocking issue.
    // You can still file a report, it'll simply default to unspecified.

    ui.m_messageWidget->setText(i18nc("@info error when talking to the bugzilla API", "An error occurred when talking to bugs.kde.org: %1", error));
    ui.m_messageWidget->animatedShow();

    ui.m_distroChooserCombo->setVisible(true);
    ui.m_distroChooserCombo->setDisabled(true);
}

bool BugzillaInformationPage::showNextPage()
{
    checkTexts();

    if (m_textsOK) {
        bool detailsShort = currentDescriptionCharactersCount() < m_requiredCharacters;

        if (detailsShort) {
            // The user input is less than we want.... encourage to write more
            QString message = i18nc("@info",
                                    "The description about the crash details does not provide "
                                    "enough information yet.<br /><br />");

            message += QLatin1Char(' ')
                + i18nc("@info",
                        "The amount of required information is proportional to "
                        "the quality of the other information like the backtrace "
                        "or the reproducibility rate."
                        "<br /><br />");

            if (reportInterface()->userCanProvideActionsAppDesktop() || reportInterface()->userCanProvideUnusualBehavior()
                || reportInterface()->userCanProvideApplicationConfigDetails()) {
                message += QLatin1Char(' ')
                    + i18nc("@info",
                            "Previously, you told DrKonqi that you could provide some "
                            "contextual information. Try writing more details about your situation. "
                            "(even little ones could help us.)<br /><br />");
            }

            message += QLatin1Char(' ')
                + i18nc("@info",
                        "If you cannot provide more information, your report "
                        "will probably waste developers' time. Can you tell us more?");

            KGuiItem yesItem = KStandardGuiItem::yes();
            yesItem.setText(i18n("Yes, let me add more information"));

            KGuiItem noItem = KStandardGuiItem::no();
            noItem.setText(i18n("No, I cannot add any other information"));

            if (KMessageBox::warningYesNo(this, message, i18nc("@title:window", "We need more information"), yesItem, noItem) == KMessageBox::No) {
                // Request the assistant to close itself (it will prompt for confirmation anyways)
                assistant()->close();
                return false;
            }
        } else {
            return true;
        }
    }

    return false;
}

bool BugzillaInformationPage::isComplete()
{
    return m_textsOK;
}

void BugzillaInformationPage::aboutToHide()
{
    // Save fields data
    reportInterface()->setTitle(ui.m_titleEdit->text());
    reportInterface()->setDetailText(ui.m_detailsEdit->toPlainText());

    if (ui.m_distroChooserCombo->isVisible()) {
        // Save bugzilla platform (distribution)
        const QString bugzillaPlatform = ui.m_distroChooserCombo->itemText(ui.m_distroChooserCombo->currentIndex());
        KConfigGroup config(KSharedConfig::openConfig(), "BugzillaInformationPage");
        config.writeEntry("BugzillaPlatform", bugzillaPlatform);
        DrKonqi::systemInformation()->setBugzillaPlatform(bugzillaPlatform);
    }
    bool compiledFromSources = ui.m_compiledSourcesCheckBox->checkState() == Qt::Checked;
    DrKonqi::systemInformation()->setCompiledSources(compiledFromSources);
}

void BugzillaInformationPage::showTitleExamples()
{
    QString titleExamples = xi18nc("@info:tooltip examples of good bug report titles",
                                   "<strong>Examples of good titles:</strong><nl />\"Plasma crashed after adding the Notes "
                                   "widget and writing on it\"<nl />\"Konqueror crashed when accessing the Facebook "
                                   "application 'X'\"<nl />\"Kopete suddenly closed after resuming the computer and "
                                   "talking to a MSN buddy\"<nl />\"Kate closed while editing a log file and pressing the "
                                   "Delete key a couple of times\"");
    QToolTip::showText(QCursor::pos(), titleExamples);
}

void BugzillaInformationPage::showDescriptionHelpExamples()
{
    QString descriptionHelp =
        i18nc("@info:tooltip help and examples of good bug descriptions", "Describe in as much detail as possible the crash circumstances:");
    if (reportInterface()->userCanProvideActionsAppDesktop()) {
        descriptionHelp += QLatin1String("<br />")
            + i18nc("@info:tooltip help and examples of good bug descriptions",
                    "- Detail which actions were you taking inside and outside the "
                    "application an instant before the crash.");
    }
    if (reportInterface()->userCanProvideUnusualBehavior()) {
        descriptionHelp += QLatin1String("<br />")
            + i18nc("@info:tooltip help and examples of good bug descriptions",
                    "- Note if you noticed any unusual behavior in the application "
                    "or in the whole environment.");
    }
    if (reportInterface()->userCanProvideApplicationConfigDetails()) {
        descriptionHelp += QLatin1String("<br />")
            + i18nc("@info:tooltip help and examples of good bug descriptions", "- Note any non-default configuration in the application.");
        if (reportInterface()->appDetailsExamples()->hasExamples()) {
            descriptionHelp += QLatin1Char(' ')
                + i18nc("@info:tooltip examples of configuration details. "
                        "the examples are already translated",
                        "Examples: %1",
                        reportInterface()->appDetailsExamples()->examples());
        }
    }
    QToolTip::showText(QCursor::pos(), descriptionHelp);
}

// END BugzillaInformationPage

// BEGIN BugzillaPreviewPage

BugzillaPreviewPage::BugzillaPreviewPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
{
    ui.setupUi(this);
}

void BugzillaPreviewPage::aboutToShow()
{
    ui.m_previewEdit->setText(reportInterface()->generateReportFullText(ReportInterface::DrKonqiStamp::Include, ReportInterface::Backtrace::Complete));
    assistant()->setAboutToSend(true);
}

void BugzillaPreviewPage::aboutToHide()
{
    assistant()->setAboutToSend(false);
}

// END BugzillaPreviewPage

// BEGIN BugzillaSendPage

BugzillaSendPage::BugzillaSendPage(ReportAssistantDialog *parent)
    : ReportAssistantPage(parent)
    , m_contentsDialog(nullptr)
{
    connect(reportInterface(), &ReportInterface::reportSent, this, &BugzillaSendPage::sent);
    connect(reportInterface(), &ReportInterface::sendReportError, this, &BugzillaSendPage::sendError);

    ui.setupUi(this);

    KGuiItem::assign(ui.m_retryButton,
                     KGuiItem2(i18nc("@action:button", "Retry..."),
                               QIcon::fromTheme(QStringLiteral("view-refresh")),
                               i18nc("@info:tooltip",
                                     "Use this button to retry "
                                     "sending the crash report if it failed before.")));

    KGuiItem::assign(ui.m_showReportContentsButton,
                     KGuiItem2(i18nc("@action:button", "Sho&w Contents of the Report"),
                               QIcon::fromTheme(QStringLiteral("document-preview")),
                               i18nc("@info:tooltip",
                                     "Use this button to show the generated "
                                     "report information about this crash.")));
    connect(ui.m_showReportContentsButton, &QPushButton::clicked, this, &BugzillaSendPage::openReportContents);

    ui.m_retryButton->setVisible(false);
    connect(ui.m_retryButton, &QAbstractButton::clicked, this, &BugzillaSendPage::retryClicked);

    ui.m_launchPageOnFinish->setVisible(false);
    ui.m_restartAppOnFinish->setVisible(false);

    connect(assistant()->finishButton(), &QPushButton::clicked, this, &BugzillaSendPage::finishClicked);
}

void BugzillaSendPage::retryClicked()
{
    ui.m_retryButton->setEnabled(false);
    aboutToShow();
}

void BugzillaSendPage::aboutToShow()
{
    ui.m_statusWidget->setBusy(i18nc("@info:status", "Sending crash report... (please wait)"));

    // Trigger relogin. If the user took a long time to prepare the login our
    // token might have gone invalid in the meantime. As a cheap way to prevent
    // this we'll simply refresh the token regardless. It's plenty cheap and
    // should reliably ensure that the token is current.
    // Disconnect everything first though, this function may get called a bunch
    // of times, so we don't want duplicated submissions.
    disconnect(bugzillaManager(), &BugzillaManager::loginFinished, reportInterface(), &ReportInterface::sendBugReport);
    disconnect(bugzillaManager(), &BugzillaManager::loginError, this, nullptr);
    connect(bugzillaManager(), &BugzillaManager::loginFinished, reportInterface(), &ReportInterface::sendBugReport);
    connect(bugzillaManager(), &BugzillaManager::loginError, this, &BugzillaSendPage::sendError);
    bugzillaManager()->refreshToken();
}

void BugzillaSendPage::sent(int bug_id)
{
    // Disconnect login->submit chain again.
    disconnect(bugzillaManager(), &BugzillaManager::loginFinished, reportInterface(), &ReportInterface::sendBugReport);
    disconnect(bugzillaManager(), &BugzillaManager::loginError, this, nullptr);

    ui.m_statusWidget->setVisible(false);
    ui.m_retryButton->setEnabled(false);
    ui.m_retryButton->setVisible(false);

    ui.m_showReportContentsButton->setVisible(false);

    ui.m_launchPageOnFinish->setVisible(true);
    ui.m_restartAppOnFinish->setVisible(!DrKonqi::crashedApplication()->hasBeenRestarted());
    ui.m_restartAppOnFinish->setChecked(false);

    reportUrl = bugzillaManager()->urlForBug(bug_id);
    ui.m_finishedLabel->setText(xi18nc("@info/rich",
                                       "Crash report sent.<nl/>"
                                       "URL: <link>%1</link><nl/>"
                                       "Thank you for being part of KDE. "
                                       "You can now close this window.",
                                       reportUrl));

    Q_EMIT finished(false);
}

void BugzillaSendPage::sendError(const QString &errorString)
{
    ui.m_statusWidget->setIdle(xi18nc("@info:status",
                                      "Error sending the crash report:  "
                                      "<message>%1.</message>",
                                      errorString));

    ui.m_retryButton->setEnabled(true);
    ui.m_retryButton->setVisible(true);
}

void BugzillaSendPage::finishClicked()
{
    if (ui.m_launchPageOnFinish->isChecked() && !reportUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(reportUrl));
    }
    if (ui.m_restartAppOnFinish->isChecked()) {
        DrKonqi::crashedApplication()->restart();
    }
}

void BugzillaSendPage::openReportContents()
{
    if (!m_contentsDialog) {
        QString report = reportInterface()->generateReportFullText(ReportInterface::DrKonqiStamp::Exclude, ReportInterface::Backtrace::Complete)
            + QLatin1Char('\n') + i18nc("@info report to KDE bugtracker address", "Report to %1", DrKonqi::crashedApplication()->bugReportAddress());
        m_contentsDialog = new ReportInformationDialog(report);
    }
    m_contentsDialog->show();
    m_contentsDialog->raise();
    m_contentsDialog->activateWindow();
}

// END BugzillaSendPage
