/*******************************************************************
 * drkonqidialog.cpp
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "drkonqidialog.h"

#include <KLocalizedString>
#include <KWindowConfig>

#include "drkonqi_debug.h"
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QLocale>
#include <QMenu>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>

#include "aboutbugreportingdialog.h"
#include "backtracewidget.h"
#include "bugzillaintegration/reportassistantdialog.h"
#include "config-drkonqi.h"
#include "crashedapplication.h"
#include "debuggerlaunchers.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqi_globals.h"

static const char ABOUT_BUG_REPORTING_URL[] = "#aboutbugreporting";
static QString DRKONQI_REPORT_BUG_URL = KDE_BUGZILLA_URL + QStringLiteral("enter_bug.cgi?product=drkonqi&format=guided");

DrKonqiDialog::DrKonqiDialog(QWidget *parent)
    : QDialog(parent)
    , m_aboutBugReportingDialog(nullptr)
    , m_backtraceWidget(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    // Setting dialog title and icon
    setWindowTitle(DrKonqi::crashedApplication()->name());
    setWindowIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug")));

    QVBoxLayout *l = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);
    l->addWidget(m_tabWidget);
    m_buttonBox = new QDialogButtonBox(this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::rejected);
    l->addWidget(m_buttonBox);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &DrKonqiDialog::tabIndexChanged);

    KConfigGroup config(KSharedConfig::openConfig(), "General");

    if (!config.readEntry(QStringLiteral("ShowOnlyBacktrace"), false)) {
        buildIntroWidget();
        m_tabWidget->addTab(m_introWidget, i18nc("@title:tab general information", "&General"));
    }

    m_backtraceWidget = new BacktraceWidget(DrKonqi::debuggerManager()->backtraceGenerator(), this);
    m_tabWidget->addTab(m_backtraceWidget, i18nc("@title:tab", "&Developer Information"));

    m_tabWidget->tabBar()->setVisible(m_tabWidget->count() > 1);

    buildDialogButtons();

    KWindowConfig::restoreWindowSize(windowHandle(), config);

    // Force a 16:9 ratio for nice appearance by default.
    QSize aspect(16, 9);
    aspect.scale(sizeHint(), Qt::KeepAspectRatioByExpanding);
    resize(aspect);
}

DrKonqiDialog::~DrKonqiDialog()
{
    KConfigGroup config(KSharedConfig::openConfig(), "General");
    KWindowConfig::saveWindowSize(windowHandle(), config);
}

void DrKonqiDialog::tabIndexChanged(int index)
{
    if (index == m_tabWidget->indexOf(m_backtraceWidget)) {
        m_backtraceWidget->generateBacktrace();
    }
}

void DrKonqiDialog::buildIntroWidget()
{
    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    m_introWidget = new QWidget(this);
    ui.setupUi(m_introWidget);

    ui.titleLabel->setText(xi18nc("@info",
                                  "<para>We are sorry, <application>%1</application> "
                                  "closed unexpectedly.</para>",
                                  crashedApp->name()));

    QString reportMessage;
    if (!crashedApp->bugReportAddress().isEmpty()) {
        if (crashedApp->fakeExecutableBaseName() == QLatin1String("drkonqi")) { // Handle own crashes
            reportMessage = xi18nc("@info",
                                   "<para>As the Crash Handler itself has failed, the "
                                   "automatic reporting process is disabled to reduce the "
                                   "risks of failing again.<nl /><nl />"
                                   "Please, <link url='%1'>manually report</link> this error "
                                   "to the KDE bug tracking system. Do not forget to include "
                                   "the backtrace from the <interface>Developer Information</interface> "
                                   "tab.</para>",
                                   DRKONQI_REPORT_BUG_URL);
        } else if (DrKonqi::isSafer()) {
            reportMessage = xi18nc("@info",
                                   "<para>The reporting assistant is disabled because "
                                   "the crash handler dialog was started in safe mode."
                                   "<nl />You can manually report this bug to %1 "
                                   "(including the backtrace from the "
                                   "<interface>Developer Information</interface> "
                                   "tab.)</para>",
                                   crashedApp->bugReportAddress());
        } else if (crashedApp->hasDeletedFiles()) {
            reportMessage = xi18nc("@info",
                                   "<para>The reporting assistant is disabled because "
                                   "the crashed application appears to have been updated or "
                                   "uninstalled since it had been started. This prevents accurate "
                                   "crash reporting and can also be the cause of this crash.</para>"
                                   "<para>After updating it is always a good idea to log out and back "
                                   "in to make sure the update is fully applied and will not cause "
                                   "any side effects.</para>");
        } else {
            reportMessage = xi18nc("@info",
                                   "<para>You can help us improve KDE Software by reporting "
                                   "this error.<nl /><link url='%1'>Learn "
                                   "more about bug reporting.</link></para>",
                                   QLatin1String(ABOUT_BUG_REPORTING_URL));
        }
    } else {
        reportMessage = xi18nc("@info",
                               "<para>You cannot report this error, because "
                               "<application>%1</application> does not provide a bug reporting "
                               "address.</para>",
                               crashedApp->name());
    }
    ui.infoLabel->setText(reportMessage);
    connect(ui.infoLabel, &QLabel::linkActivated, this, &DrKonqiDialog::linkActivated);

    ui.detailsTitleLabel->setText(QStringLiteral("<strong>%1</strong>").arg(i18nc("@label", "Details:")));

    QLocale locale;
    ui.detailsLabel->setText(xi18nc("@info Note the time information is divided into date and time parts",
                                    "<para>Executable: <application>%1"
                                    "</application> PID: %2 Signal: %3 (%4) "
                                    "Time: %5 %6</para>",
                                    crashedApp->fakeExecutableBaseName(),
                                    // prevent number localization by ki18n
                                    QString::number(crashedApp->pid()),
                                    crashedApp->signalName(),
#if defined(Q_OS_UNIX)
                                    // prevent number localization by ki18n
                                    QString::number(crashedApp->signalNumber()),
#else
                                    // windows uses weird big numbers for exception codes,
                                    // so it doesn't make sense to display them in decimal
                                    QString().asprintf("0x%8x", crashedApp->signalNumber()),
#endif
                                    locale.toString(crashedApp->datetime().date(), QLocale::ShortFormat),

                                    locale.toString(crashedApp->datetime().time())));
}

void DrKonqiDialog::buildDialogButtons()
{
    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    // Set dialog buttons
    m_buttonBox->setStandardButtons(QDialogButtonBox::Close);

    // Default debugger button and menu (only for developer mode): User2
    DebuggerManager *debuggerManager = DrKonqi::debuggerManager();
    m_debugButton = new QPushButton(this);
    KGuiItem2 debugItem(i18nc("@action:button this is the debug menu button label which contains the debugging applications", "&Debug"),
                        QIcon::fromTheme(QStringLiteral("applications-development")),
                        i18nc("@info:tooltip", "Starts a program to debug the crashed application."));
    KGuiItem::assign(m_debugButton, debugItem);
    m_debugButton->setVisible(false);

    m_debugMenu = new QMenu(this);
    m_debugButton->setMenu(m_debugMenu);

    // Only add the debugger if requested by the config or if a KDevelop session is running.
    const bool showExternal = debuggerManager->showExternalDebuggers();
    QList<AbstractDebuggerLauncher *> debuggers = debuggerManager->availableExternalDebuggers();
    foreach (AbstractDebuggerLauncher *launcher, debuggers) {
        if (showExternal || qobject_cast<DBusInterfaceLauncher *>(launcher)) {
            addDebugger(launcher);
        }
    }

    connect(debuggerManager, &DebuggerManager::externalDebuggerAdded, this, &DrKonqiDialog::addDebugger);
    connect(debuggerManager, &DebuggerManager::externalDebuggerRemoved, this, &DrKonqiDialog::removeDebugger);
    connect(debuggerManager, &DebuggerManager::debuggerRunning, this, &DrKonqiDialog::enableDebugMenu);

    // Report bug button: User1
    QPushButton *reportButton = new QPushButton(m_buttonBox);
    KGuiItem2 reportItem(i18nc("@action:button", "Report &Bug"),
                         QIcon::fromTheme(QStringLiteral("tools-report-bug")),
                         i18nc("@info:tooltip", "Starts the bug report assistant."));
    KGuiItem::assign(reportButton, reportItem);
    m_buttonBox->addButton(reportButton, QDialogButtonBox::ActionRole);

    reportButton->setEnabled(!crashedApp->bugReportAddress().isEmpty() && crashedApp->fakeExecutableBaseName() != QLatin1String("drkonqi")
                             && !DrKonqi::isSafer() && !crashedApp->hasDeletedFiles());
    connect(reportButton, &QPushButton::clicked, this, &DrKonqiDialog::startBugReportAssistant);

    // Restart application button
    KGuiItem2 restartItem(i18nc("@action:button", "&Restart Application"),
                          QIcon::fromTheme(QStringLiteral("system-reboot")),
                          i18nc("@info:tooltip",
                                "Use this button to restart "
                                "the crashed application."));
    m_restartButton = new QPushButton(m_buttonBox);
    KGuiItem::assign(m_restartButton, restartItem);
    m_restartButton->setEnabled(!crashedApp->hasBeenRestarted() && crashedApp->fakeExecutableBaseName() != QLatin1String("drkonqi"));
    m_buttonBox->addButton(m_restartButton, QDialogButtonBox::ActionRole);
    connect(m_restartButton, &QAbstractButton::clicked, crashedApp, &CrashedApplication::restart);
    connect(crashedApp, &CrashedApplication::restarted, this, &DrKonqiDialog::applicationRestarted);

    // Close button
    QString tooltipText = i18nc("@info:tooltip", "Close this dialog (you will lose the crash information.)");
    m_buttonBox->button(QDialogButtonBox::Close)->setToolTip(tooltipText);
    m_buttonBox->button(QDialogButtonBox::Close)->setWhatsThis(tooltipText);
    m_buttonBox->button(QDialogButtonBox::Close)->setFocus();
}

void DrKonqiDialog::addDebugger(AbstractDebuggerLauncher *launcher)
{
    QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("applications-development")),
                                  i18nc("@action:inmenu 1 is the debugger name", "Debug in %1", launcher->name()),
                                  m_debugMenu);
    m_debugMenu->addAction(action);
    connect(action, &QAction::triggered, launcher, &AbstractDebuggerLauncher::start);
    m_debugMenuActions.insert(launcher, action);

    // Make sure that the debug button is the first button with action role to be
    // inserted, then add the other buttons. See removeDebugger below for more information.
    if (!m_debugButtonInBox) {
        auto buttons = m_buttonBox->buttons();
        m_buttonBox->addButton(m_debugButton, QDialogButtonBox::ActionRole);
        m_debugButton->setVisible(true);
        for (QAbstractButton *button : buttons) {
            if (m_buttonBox->buttonRole(button) == QDialogButtonBox::ActionRole) {
                m_buttonBox->addButton(button, QDialogButtonBox::ActionRole);
            }
        }
        m_debugButtonInBox = true;
    }
}

void DrKonqiDialog::removeDebugger(AbstractDebuggerLauncher *launcher)
{
    QAction *action = m_debugMenuActions.take(launcher);
    if (action) {
        m_debugMenu->removeAction(action);
        action->deleteLater();
        // Remove the button from the box, otherwise the box will force
        // it visible as it calls show() explicitly. (QTBUG-3651)
        if (m_debugMenu->isEmpty()) {
            m_buttonBox->removeButton(m_debugButton);
            m_debugButton->setVisible(false);
            m_debugButton->setParent(this);
            m_debugButtonInBox = false;
        }
    } else {
        qCWarning(DRKONQI_LOG) << "Invalid launcher";
    }
}

void DrKonqiDialog::enableDebugMenu(bool debuggerRunning)
{
    m_debugButton->setEnabled(!debuggerRunning);
}

void DrKonqiDialog::startBugReportAssistant()
{
    ReportAssistantDialog *bugReportAssistant = new ReportAssistantDialog();
    bugReportAssistant->show();
    connect(bugReportAssistant, &QObject::destroyed, this, &DrKonqiDialog::reject);

    hide();
}

void DrKonqiDialog::linkActivated(const QString &link)
{
    if (link == QLatin1String(ABOUT_BUG_REPORTING_URL)) {
        showAboutBugReporting();
    } else if (link == DRKONQI_REPORT_BUG_URL) {
        QDesktopServices::openUrl(QUrl(link));
    } else if (link.startsWith(QLatin1String("http"))) {
        qCWarning(DRKONQI_LOG) << "unexpected link";
        QDesktopServices::openUrl(QUrl(link));
    }
}

void DrKonqiDialog::showAboutBugReporting()
{
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog();
        connect(this, &DrKonqiDialog::destroyed, m_aboutBugReportingDialog.data(), &AboutBugReportingDialog::close);
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->raise();
    m_aboutBugReportingDialog->activateWindow();
}

void DrKonqiDialog::applicationRestarted(bool success)
{
    m_restartButton->setEnabled(!success);
}
