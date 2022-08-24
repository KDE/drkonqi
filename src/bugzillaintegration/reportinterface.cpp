/*******************************************************************
 * reportinterface.cpp
 * SPDX-FileCopyrightText: 2009, 2010, 2011 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 * SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
 * SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#include "reportinterface.h"

#include <chrono>

#include <KIO/TransferJob>
#include <KLocalizedString>
#include <KUserFeedback/Provider>

#include "backtracegenerator.h"
#include "bugzillalib.h"
#include "config-drkonqi.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqi_debug.h"
#include "parser/backtraceparser.h"
#include "productmapping.h"
#include "systeminformation.h"

// Max size a report may have. This is enforced in bugzilla, hardcoded, and
// cannot be queried through the API, so handle this client-side in a hardcoded
// fashion as well.
static const int s_maxReportSize = 65535;

ReportInterface::ReportInterface(QObject *parent)
    : QObject(parent)
    , m_duplicate(0)
{
    m_bugzillaManager = new BugzillaManager(KDE_BUGZILLA_URL, this);

    m_productMapping = new ProductMapping(DrKonqi::crashedApplication(), m_bugzillaManager, this);

    // Information the user can provide about the crash
    m_userRememberCrashSituation = false;
    m_reproducible = ReproducibleUnsure;
    m_provideActionsApplicationDesktop = false;
    m_provideUnusualBehavior = false;
    m_provideApplicationConfigurationDetails = false;

    // Do not attach the bug report to any other existent report (create a new one)
    m_attachToBugNumber = 0;

    connect(&m_sentryBeacon, &SentryBeacon::eventSent, this, [this] {
        m_sentryEventSent = true;
        maybeDone();
    });
    connect(&m_sentryBeacon, &SentryBeacon::userFeedbackSent, this, [this] {
        m_sentryUserFeedbackSent = true;
        maybeDone();
    });
    if (KUserFeedback::Provider provider; provider.isEnabled() && !DrKonqi::isTestingBugzilla() && !DrKonqi::crashedApplication()->hasDeletedFiles()) {
        metaObject()->invokeMethod(this, [this] {
            // Send crash event ASAP, if applicable. Trace quality doesn't matter for it.
            sendCrashEvent();
        });
    }
}

void ReportInterface::setBugAwarenessPageData(bool rememberSituation, Reproducible reproducible, bool actions, bool unusual, bool configuration)
{
    // Save the information the user can provide about the crash from the assistant page
    m_userRememberCrashSituation = rememberSituation;
    m_reproducible = reproducible;
    m_provideActionsApplicationDesktop = actions;
    m_provideUnusualBehavior = unusual;
    m_provideApplicationConfigurationDetails = configuration;
}

bool ReportInterface::isBugAwarenessPageDataUseful() const
{
    // Determine if the assistant should proceed, considering the amount of information
    // the user can provide
    int rating = selectedOptionsRating();

    // Minimum information required even for a good backtrace.
    bool useful = m_userRememberCrashSituation && (rating >= 2 || (m_reproducible == ReproducibleSometimes || m_reproducible == ReproducibleEverytime));
    return useful;
}

int ReportInterface::selectedOptionsRating() const
{
    // Check how many information the user can provide and generate a rating
    int rating = 0;
    if (m_provideActionsApplicationDesktop) {
        rating += 3;
    }
    if (m_provideApplicationConfigurationDetails) {
        rating += 2;
    }
    if (m_provideUnusualBehavior) {
        rating += 1;
    }
    return rating;
}

QString ReportInterface::backtrace() const
{
    return m_backtrace;
}

void ReportInterface::setBacktrace(const QString &backtrace)
{
    m_backtrace = backtrace;
    Q_EMIT backtraceChanged();
}

QStringList ReportInterface::firstBacktraceFunctions() const
{
    return m_firstBacktraceFunctions;
}

void ReportInterface::setFirstBacktraceFunctions(const QStringList &functions)
{
    m_firstBacktraceFunctions = functions;
}

QString ReportInterface::title() const
{
    return m_reportTitle;
}

void ReportInterface::setTitle(const QString &text)
{
    m_reportTitle = text;
    Q_EMIT titleChanged();
}

void ReportInterface::setDetailText(const QString &text)
{
    m_reportDetailText = text;
    Q_EMIT detailTextChanged();
}

void ReportInterface::setPossibleDuplicates(const QStringList &list)
{
    m_possibleDuplicates = list;
}

QString ReportInterface::generateReportFullText(DrKonqiStamp stamp, Backtrace inlineBacktrace) const
{
    // Note: no translations should be done in this function's strings

    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();
    const SystemInformation *sysInfo = DrKonqi::systemInformation();

    QString report;

    // Program name and versions
    report.append(QStringLiteral("Application: %1 (%2)\n").arg(crashedApp->fakeExecutableBaseName(), crashedApp->version()));
    if (sysInfo->compiledSources()) {
        report.append(QStringLiteral(" (Compiled from sources)\n"));
    } else {
        report.append(QLatin1Char('\n'));
    }
    report.append(QStringLiteral("Qt Version: %1\n").arg(sysInfo->qtVersion()));
    report.append(QStringLiteral("Frameworks Version: %1\n").arg(sysInfo->frameworksVersion()));

    report.append(QStringLiteral("Operating System: %1\n").arg(sysInfo->operatingSystem()));
    report.append(QStringLiteral("Windowing System: %1\n").arg(sysInfo->windowSystem()));

    // LSB output or manually selected distro
    if (!sysInfo->distributionPrettyName().isEmpty()) {
        report.append(QStringLiteral("Distribution: %1\n").arg(sysInfo->distributionPrettyName()));
    } else if (!sysInfo->bugzillaPlatform().isEmpty() && sysInfo->bugzillaPlatform() != QLatin1String("unspecified")) {
        report.append(QStringLiteral("Distribution (Platform): %1\n").arg(sysInfo->bugzillaPlatform()));
    }

    report.append(QStringLiteral("DrKonqi: %1 [%2]\n").arg(QString::fromLatin1(PROJECT_VERSION), DrKonqi::backendClassName()));
    report.append(QLatin1Char('\n'));

    // Details of the crash situation
    if (isBugAwarenessPageDataUseful()) {
        report.append(QStringLiteral("-- Information about the crash:\n"));
        if (!m_reportDetailText.isEmpty()) {
            report.append(m_reportDetailText.trimmed());
        } else {
            // If the user manual reports this crash, he/she should know what to put in here.
            // This message is the only one translated in this function
            report.append(xi18nc("@info/plain",
                                 "<placeholder>In detail, tell us what you were doing "
                                 " when the application crashed.</placeholder>"));
        }
        report.append(QLatin1String("\n\n"));
    }

    // Crash reproducibility
    switch (m_reproducible) {
    case ReproducibleUnsure:
        report.append(QStringLiteral("The reporter is unsure if this crash is reproducible.\n\n"));
        break;
    case ReproducibleNever:
        report.append(QStringLiteral("The crash does not seem to be reproducible.\n\n"));
        break;
    case ReproducibleSometimes:
        report.append(QStringLiteral("The crash can be reproduced sometimes.\n\n"));
        break;
    case ReproducibleEverytime:
        report.append(QStringLiteral("The crash can be reproduced every time.\n\n"));
        break;
    }

    // Backtrace
    switch (inlineBacktrace) {
    case Backtrace::Complete:
        report.append(QStringLiteral("-- Backtrace:\n"));
        break;
    case Backtrace::Reduced:
        report.append(QStringLiteral("-- Backtrace (Reduced):\n"));
        break;
    case Backtrace::Exclude:
        report.append(QStringLiteral("The backtrace was excluded and likely attached as a file.\n"));
        break;
    }
    if (!m_backtrace.isEmpty()) {
        switch (inlineBacktrace) {
        case Backtrace::Complete:
            report.append(m_backtrace.trimmed() + QLatin1Char('\n'));
            break;
        case Backtrace::Reduced:
            report.append(DrKonqi::debuggerManager()->backtraceGenerator()->parser()->simplifiedBacktrace() + QLatin1Char('\n'));
            break;
        case Backtrace::Exclude:
            report.append(QStringLiteral("The backtrace is attached as a comment due to length constraints\n"));
            break;
        }
    } else {
        report.append(QStringLiteral("A useful backtrace could not be generated\n"));
    }

    // Possible duplicates (selected by the user)
    if (!m_possibleDuplicates.isEmpty()) {
        report.append(QLatin1Char('\n'));
        QString duplicatesString;
        for (const QString &dupe : std::as_const(m_possibleDuplicates)) {
            duplicatesString += QLatin1String("bug ") + dupe + QLatin1String(", ");
        }
        duplicatesString = duplicatesString.left(duplicatesString.length() - 2) + QLatin1Char('.');
        report.append(QStringLiteral("The reporter indicates this bug may be a duplicate of or related to %1\n").arg(duplicatesString));
    }

    // Several possible duplicates (by bugzilla query)
    if (!m_allPossibleDuplicatesByQuery.isEmpty()) {
        report.append(QLatin1Char('\n'));
        QString duplicatesString;
        int count = m_allPossibleDuplicatesByQuery.count();
        for (int i = 0; i < count && i < 5; i++) {
            duplicatesString += QLatin1String("bug ") + m_allPossibleDuplicatesByQuery.at(i) + QLatin1String(", ");
        }
        duplicatesString = duplicatesString.left(duplicatesString.length() - 2) + QLatin1Char('.');
        report.append(QStringLiteral("Possible duplicates by query: %1\n").arg(duplicatesString));
    }

    switch (stamp) {
    case DrKonqiStamp::Include: {
        report.append(QLatin1String("\nReported using DrKonqi"));
        const QString product = m_productMapping->bugzillaProduct();
        const QString originalProduct = m_productMapping->bugzillaProductOriginal();
        if (!originalProduct.isEmpty()) {
            report.append(
                QStringLiteral(
                    "\nThis report was filed against '%1' because the product '%2' could not be located in Bugzilla. Add it to drkonqi's mappings file!")
                    .arg(product, originalProduct));
        }
        break;
    }
    case DrKonqiStamp::Exclude:
        break;
    }

    return report;
}

QString ReportInterface::generateAttachmentComment() const
{
    // Note: no translations should be done in this function's strings

    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();
    const SystemInformation *sysInfo = DrKonqi::systemInformation();

    QString comment;

    // Program name and versions
    comment.append(QStringLiteral("%1 (%2) using Qt %4\n\n").arg(crashedApp->fakeExecutableBaseName(), crashedApp->version(), sysInfo->qtVersion()));

    // Details of the crash situation
    if (isBugAwarenessPageDataUseful()) {
        comment.append(QStringLiteral("%1\n\n").arg(m_reportDetailText.trimmed()));
    }

    // Backtrace (only 6 lines)
    comment.append(QStringLiteral("-- Backtrace (Reduced):\n"));
    QString reducedBacktrace = DrKonqi::debuggerManager()->backtraceGenerator()->parser()->simplifiedBacktrace();
    comment.append(reducedBacktrace.trimmed());

    return comment;
}

Bugzilla::NewBug ReportInterface::newBugReportTemplate() const
{
    const SystemInformation *sysInfo = DrKonqi::systemInformation();

    Bugzilla::NewBug bug;
    bug.product = m_productMapping->bugzillaProduct();
    bug.component = m_productMapping->bugzillaComponent();
    bug.version = m_productMapping->bugzillaVersion();
    bug.op_sys = sysInfo->bugzillaOperatingSystem();
    if (sysInfo->compiledSources()) {
        bug.platform = QLatin1String("Compiled Sources");
    } else {
        bug.platform = sysInfo->bugzillaPlatform();
    }
    bug.keywords = QStringList{QStringLiteral("drkonqi")};
    bug.priority = QLatin1String("NOR");
    bug.severity = QLatin1String("crash");
    bug.summary = m_reportTitle;

    return bug;
}

void ReportInterface::sendCrashEvent()
{
#ifdef WITH_SENTRY
    if (DrKonqi::debuggerManager()->backtraceGenerator()->state() == BacktraceGenerator::Loaded) {
        m_sentryBeacon.sendEvent();
        return;
    }
    static bool connected = false;
    if (!connected) {
        connected = true;
        connect(DrKonqi::debuggerManager()->backtraceGenerator(), &BacktraceGenerator::done, this, [this] {
            m_sentryBeacon.sendEvent();
        });
    }
    if (DrKonqi::debuggerManager()->backtraceGenerator()->state() != BacktraceGenerator::Loading) {
        DrKonqi::debuggerManager()->backtraceGenerator()->start();
    }
#endif
}

void ReportInterface::sendCrashComment()
{
#ifdef WITH_SENTRY
    m_sentryBeacon.sendUserFeedback(m_reportTitle + QLatin1Char('\n') + m_reportDetailText + QLatin1Char('\n') + DrKonqi::kdeBugzillaURL()
                                    + QLatin1String("show_bug.cgi?id=%1").arg(QString::number(m_sentReport)));
#endif
}

void ReportInterface::sendBugReport()
{
    sendCrashEvent();

    if (m_attachToBugNumber > 0) {
        // We are going to attach the report to an existent one
        connect(m_bugzillaManager, &BugzillaManager::addMeToCCFinished, this, &ReportInterface::attachBacktraceWithReport);
        connect(m_bugzillaManager, &BugzillaManager::addMeToCCError, this, &ReportInterface::sendReportError);
        // First add the user to the CC list, then attach
        m_bugzillaManager->addMeToCC(m_attachToBugNumber);
    } else {
        // Creating a new bug report
        bool attach = false;
        Bugzilla::NewBug report = newBugReportTemplate();
        report.description = generateReportFullText(ReportInterface::DrKonqiStamp::Include, ReportInterface::Backtrace::Complete);

        // If the report is too long try to reduce it, try to not include the
        // backtrace and eventually give up.
        // Bugzilla has a hard-limit on the server side, if we cannot strip the
        // report down enough the submission will simply not work.
        // Exhausting the cap with just user input is nigh impossible, so we'll
        // forego handling of the report being too long even without without
        // backtrace.
        // https://bugs.kde.org/show_bug.cgi?id=248807
        if (report.description.size() >= s_maxReportSize) {
            report.description = generateReportFullText(ReportInterface::DrKonqiStamp::Include, ReportInterface::Backtrace::Reduced);
            attach = true;
        }
        if (report.description.size() >= s_maxReportSize) {
            report.description = generateReportFullText(ReportInterface::DrKonqiStamp::Include, ReportInterface::Backtrace::Exclude);
            attach = true;
        }
        Q_ASSERT(!report.description.isEmpty());

        connect(m_bugzillaManager, &BugzillaManager::sendReportErrorInvalidValues, this, &ReportInterface::sendUsingDefaultProduct);
        connect(m_bugzillaManager, &BugzillaManager::reportSent, this, [=](int bugId) {
            if (attach) {
                m_attachToBugNumber = bugId;
                attachBacktrace(QStringLiteral("DrKonqi auto-attaching complete backtrace."));
            } else {
                m_sentReport = bugId;
                sendCrashComment();
                maybeDone();
            }
        });
        connect(m_bugzillaManager, &BugzillaManager::sendReportError, this, &ReportInterface::sendReportError);
        m_bugzillaManager->sendReport(report);
    }
}

void ReportInterface::sendUsingDefaultProduct() const
{
    // Fallback function: if some of the custom values fail, we need to reset all the fields to the default
    //(and valid) bugzilla values; and try to resend
    Bugzilla::NewBug bug = newBugReportTemplate();
    bug.product = QLatin1String("kde");
    bug.component = QLatin1String("general");
    bug.platform = QLatin1String("unspecified");
    bug.description = generateReportFullText(ReportInterface::DrKonqiStamp::Include, ReportInterface::Backtrace::Complete);
    m_bugzillaManager->sendReport(bug);
}

void ReportInterface::attachBacktraceWithReport()
{
    attachBacktrace(generateAttachmentComment());
}

void ReportInterface::attachBacktrace(const QString &comment)
{
    // The user was added to the CC list, proceed with the attachment
    connect(m_bugzillaManager, &BugzillaManager::attachToReportSent, this, &ReportInterface::attachSent);
    connect(m_bugzillaManager, &BugzillaManager::attachToReportError, this, &ReportInterface::sendReportError);

    QString reportText = generateReportFullText(ReportInterface::DrKonqiStamp::Include, ReportInterface::Backtrace::Complete);
    QString filename = getSuggestedKCrashFilename(DrKonqi::crashedApplication());
    QLatin1String summary("New crash information added by DrKonqi");

    // Attach the report. The comment of the attachment also includes the bug description
    m_bugzillaManager->attachTextToReport(reportText, filename, summary, m_attachToBugNumber, comment);
}

void ReportInterface::attachSent(int attachId)
{
    Q_UNUSED(attachId);

    // The bug was attached, consider it "sent"
    m_sentReport = attachId;
    sendCrashComment();
    maybeDone();
}

QStringList ReportInterface::relatedBugzillaProducts() const
{
    return m_productMapping->relatedBugzillaProducts();
}

bool ReportInterface::isWorthReporting() const
{
    if (DrKonqi::ignoreQuality()) {
        return true;
    }

    // Evaluate if the provided information is useful enough to enable the automatic report
    bool needToReport = false;

    if (!m_userRememberCrashSituation) {
        // This should never happen... but...
        return false;
    }

    int rating = selectedOptionsRating();

    BacktraceParser::Usefulness use = DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();

    switch (use) {
    case BacktraceParser::ReallyUseful: {
        // Perfect backtrace: require at least one option or a 100%-50% reproducible crash
        needToReport = (rating >= 2) || (m_reproducible == ReproducibleEverytime || m_reproducible == ReproducibleSometimes);
        break;
    }
    case BacktraceParser::MayBeUseful: {
        // Not perfect backtrace: require at least two options or a 100% reproducible crash
        needToReport = (rating >= 3) || (m_reproducible == ReproducibleEverytime);
        break;
    }
    case BacktraceParser::ProbablyUseless:
        // Bad backtrace: require at least two options and always reproducible (strict)
        needToReport = (rating >= 5) && (m_reproducible == ReproducibleEverytime);
        break;
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        needToReport = false;
    }
    }

    return needToReport;
}

void ReportInterface::setAttachToBugNumber(uint bugNumber)
{
    // If bugNumber>0, the report is going to be attached to bugNumber
    m_attachToBugNumber = bugNumber;
    Q_EMIT attachToBugNumberChanged();
}

uint ReportInterface::attachToBugNumber() const
{
    return m_attachToBugNumber;
}

void ReportInterface::setDuplicateId(uint duplicate)
{
    m_duplicate = duplicate;
    Q_EMIT duplicateIdChanged();
}

uint ReportInterface::duplicateId() const
{
    return m_duplicate;
}

void ReportInterface::setPossibleDuplicatesByQuery(const QStringList &list)
{
    m_allPossibleDuplicatesByQuery = list;
}

BugzillaManager *ReportInterface::bugzillaManager() const
{
    return m_bugzillaManager;
}

ProductMapping *ReportInterface::productMapping() const
{
    return m_productMapping;
}

void ReportInterface::maybeDone()
{
    if (m_sentReport != 0 && m_sentryEventSent && m_sentryUserFeedbackSent) {
        Q_EMIT done();
    }
};
