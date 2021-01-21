/*******************************************************************
 * reportassistantpages_bugzilla.h
 * SPDX-FileCopyrightText: 2009, 2011 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef REPORTASSISTANTPAGES__BUGZILLA__H
#define REPORTASSISTANTPAGES__BUGZILLA__H

#include "reportassistantpage.h"

#include "reportassistantpages_base.h"

#include "ui_assistantpage_bugzilla_information.h"
#include "ui_assistantpage_bugzilla_login.h"
#include "ui_assistantpage_bugzilla_preview.h"
#include "ui_assistantpage_bugzilla_send.h"

#include <clients/bugfieldclient.h>

namespace KWallet
{
class Wallet;
}
class KCapacityBar;

/** Bugzilla login **/
class BugzillaLoginPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaLoginPage(ReportAssistantDialog *);
    ~BugzillaLoginPage() override;

    void aboutToShow() override;
    bool isComplete() override;

private Q_SLOTS:
    void loginClicked();
    bool canLogin() const;
    void login();
    void loginFinished(bool);
    void loginError(const QString &error);

    void walletLogin();

    void updateLoginButtonStatus();

Q_SIGNALS:
    void loggedTurnToNextPage();

private:
    void updateWidget(bool enabled);
    bool kWalletEntryExists(const QString &);
    void openWallet();

    Ui::AssistantPageBugzillaLogin ui;

    KWallet::Wallet *m_wallet;
    bool m_walletWasOpenedBefore;
    bool m_bugzillaVersionFound;
};

/** Title and details page **/
class BugzillaInformationPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaInformationPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

    bool isComplete() override;
    bool showNextPage() override;

private Q_SLOTS:
    void showTitleExamples();
    void showDescriptionHelpExamples();

    void checkTexts();
    void loadDistroCombo();

private:
    void setDistros(const Bugzilla::BugField::Ptr &field);
    void setDistroComboError(const QString &error);
    int currentDescriptionCharactersCount();

    Ui::AssistantPageBugzillaInformation ui;
    KCapacityBar *m_textCompleteBar;

    bool m_textsOK;

    int m_requiredCharacters;
};

/** Preview report page **/
class BugzillaPreviewPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaPreviewPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

private:
    Ui::AssistantPageBugzillaPreview ui;
};

/** Send crash report page **/
class BugzillaSendPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaSendPage(ReportAssistantDialog *);

    void aboutToShow() override;

private Q_SLOTS:
    void sent(int);
    void sendError(const QString &errorString);

    void retryClicked();
    void finishClicked();

    void openReportContents();

private:
    Ui::AssistantPageBugzillaSend ui;
    QString reportUrl;

    QPointer<QDialog> m_contentsDialog;

Q_SIGNALS:
    void finished(bool);
};

#endif
