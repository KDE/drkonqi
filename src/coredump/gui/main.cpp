// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>
#include <KWindowSystem>

#include <config-drkonqi.h>

#include "Patient.h"
#include "PatientModel.h"
#include "backtracegenerator.h"
#include "bugzillaintegration/reportinterface.h"
#include "drkonqi.h"
#include "settings.h"
#include "statusnotifier.h"

using namespace std::chrono_literals;

using namespace Qt::StringLiterals;

const QCommandLineOption signalOption(QStringLiteral("signal"), i18nc("@info:shell", "The signal <number> that was caught"), QStringLiteral("number"));
const QCommandLineOption appNameOption(QStringLiteral("appname"), i18nc("@info:shell", "<Name> of the program"), QStringLiteral("name"));
const QCommandLineOption appPathOption(QStringLiteral("apppath"), i18nc("@info:shell", "<Path> to the executable"), QStringLiteral("path"));
const QCommandLineOption appVersionOption(QStringLiteral("appversion"), i18nc("@info:shell", "The <version> of the program"), QStringLiteral("version"));
const QCommandLineOption bugAddressOption(QStringLiteral("bugaddress"), i18nc("@info:shell", "The bug <address> to use"), QStringLiteral("address"));
const QCommandLineOption programNameOption(QStringLiteral("programname"), i18nc("@info:shell", "Translated <name> of the program"), QStringLiteral("name"));
const QCommandLineOption productNameOption(QStringLiteral("productname"), i18nc("@info:shell", "Bugzilla product name"), QStringLiteral("name"));
const QCommandLineOption pidOption(QStringLiteral("pid"), i18nc("@info:shell", "The <PID> of the program"), QStringLiteral("pid"));
const QCommandLineOption startupIdOption(QStringLiteral("startupid"), i18nc("@info:shell", "Startup <ID> of the program"), QStringLiteral("id"));
const QCommandLineOption saferOption(QStringLiteral("safer"), i18nc("@info:shell", "Disable arbitrary disk access"));
const QCommandLineOption keepRunningOption(QStringLiteral("keeprunning"),
                                           i18nc("@info:shell", "Keep the program running and generate the backtrace at startup"));
const QCommandLineOption threadOption(QStringLiteral("thread"), i18nc("@info:shell", "The <thread id> of the failing thread"), QStringLiteral("threadid"));
const QCommandLineOption dialogOption(QStringLiteral("dialog"), i18nc("@info:shell", "Do not show a notification but launch the debug dialog directly"));
const QCommandLineOption glRendererOption(u"glrenderer"_s, u"Legacy. Unused"_s, u"glrenderer"_s);
const QCommandLineOption exceptionNameOption(u"exceptionname"_s, u"The exception class name if an exception was the cause"_s, u"name"_s);
const QCommandLineOption exceptionWhatOption(u"exceptionwhat"_s, u"The exception what string if an exception was the cause"_s, u"what"_s);
const QCommandLineOption qtVersionOption(u"qtversion"_s, u"The version of Qt used by the process"_s, u"qtversion"_s);
const QCommandLineOption frameworksVersionOption(u"kdeframeworksversion"_s, u"The KDE Frameworks version used by the process"_s, u"kdeframeworksversion"_s);
const QCommandLineOption notify(u"notify"_s, i18nc("@info", "Start the application showing only a notification that a process has crashed"));
const QCommandLineOption restartedOption(QStringLiteral("restarted"), i18nc("@info:shell", "The program has already been restarted"));
const QCommandLineOption metadataOption(QStringLiteral("metadata_file"), i18nc("@info:shell", "The path to the metadata file"), u"path"_s);

static QWindow *windowFromEngine(QQmlApplicationEngine *engine)
{
    const auto rootObjects = engine->rootObjects();
    auto *window = qobject_cast<QQuickWindow *>(rootObjects.first());
    Q_ASSERT(window);
    return window;
}

class Application
{
public:
    [[nodiscard]] bool loadUi();
    void raiseWindow();
    void requestDrKonqiDialog(bool interactionAllowed);
    int handleNotify(const QCommandLineParser &parser);

private:
    QQmlApplicationEngine *m_engine = nullptr;
};

void Application::raiseWindow()
{
    auto window = windowFromEngine(m_engine);
    KWindowSystem::updateStartupId(window);
    KWindowSystem::activateWindow(window);
}

bool Application::loadUi()
{
    m_engine = new QQmlApplicationEngine();

    auto context = new KLocalizedContext;
    context->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    m_engine->rootContext()->setContextObject(context);

    m_engine->loadFromModule("org.kde.drkonqi.coredump.gui", "Main");
    return !m_engine->rootObjects().isEmpty();
}

void Application::requestDrKonqiDialog(bool interactionAllowed)
{
    auto activation = interactionAllowed ? StatusNotifier::Activation::Allowed : StatusNotifier::Activation::NotAllowed;
    if (ReportInterface::self()->isCrashEventSendingEnabled()) {
        activation = StatusNotifier::Activation::AlreadySubmitting;
        ReportInterface::self()->setSendWhenReady(true);
        if (DrKonqi::debuggerManager()->backtraceGenerator()->state() == BacktraceGenerator::NotLoaded) {
            DrKonqi::debuggerManager()->backtraceGenerator()->start();
        }
    }

    auto statusNotifier = new StatusNotifier();
    if (interactionAllowed) {
        statusNotifier->show();
    }
    statusNotifier->notify(activation);
    QObject::connect(statusNotifier, &StatusNotifier::activated, statusNotifier, [this](auto pid) {
        PatientModel::instance()->setPatient(pid);
        PatientModel::instance()->openReport();
        if (m_engine) {
            raiseWindow();
        } else {
            if (!loadUi()) {
                exit(1);
            }
        }
    });

    QObject::connect(statusNotifier, &StatusNotifier::sentryActivated, statusNotifier, [this](auto pid) {
        PatientModel::instance()->setPatient(pid);
        PatientModel::instance()->openSentry();
        if (m_engine) {
            raiseWindow();
        } else {
            if (!loadUi()) {
                exit(1);
            }
        }
    });
}

bool isShuttingDown()
{
    if (QDBusConnection::sessionBus().isConnected()) {
        QDBusInterface remoteApp(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QStringLiteral("org.kde.KSMServerInterface"));

        QDBusReply<bool> reply = remoteApp.call(QStringLiteral("isShuttingDown"));
        return reply.isValid() ? reply.value() : false;
    }
    return false;
}

void setMetadata(const QCommandLineParser &parser)
{
    DrKonqi::setSignal(parser.value(signalOption).toInt());
    DrKonqi::setAppName(parser.value(appNameOption));
    DrKonqi::setAppPath(parser.value(appPathOption));
    DrKonqi::setAppVersion(parser.value(appVersionOption));
    DrKonqi::setBugAddress(parser.value(bugAddressOption));
    DrKonqi::setProgramName(parser.value(programNameOption));
    DrKonqi::setProductName(parser.value(productNameOption));
    DrKonqi::setPid(parser.value(pidOption).toInt());
    DrKonqi::setSafer(parser.isSet(saferOption));
    DrKonqi::setRestarted(parser.isSet(restartedOption));
    DrKonqi::setKeepRunning(parser.isSet(keepRunningOption));
    DrKonqi::setThread(parser.value(threadOption).toInt());
    DrKonqi::setStartupId(parser.value(startupIdOption));
    DrKonqi::instance()->m_exceptionName = parser.value(exceptionNameOption);
    DrKonqi::setMetadataFile(parser.value(metadataOption));
    if (!DrKonqi::instance()->m_exceptionName.isEmpty()) {
        QProcess proc;
        proc.setProgram(u"c++filt"_s);
        proc.setArguments({u"--types"_s, DrKonqi::instance()->m_exceptionName});
        proc.start();
        if (proc.waitForFinished((1000ms).count())) {
            DrKonqi::instance()->m_exceptionName = QString::fromUtf8(proc.readAllStandardOutput().trimmed());
        }
        if (DrKonqi::instance()->m_exceptionName.isEmpty()) { // restore the potentially mangled version if empty
            DrKonqi::instance()->m_exceptionName = parser.value(exceptionNameOption);
        }
    }
    DrKonqi::instance()->m_exceptionWhat = parser.value(exceptionWhatOption);
    DrKonqi::instance()->m_qtVersion = parser.value(qtVersionOption);
    DrKonqi::instance()->m_frameworksVersion = parser.value(frameworksVersionOption);

    PatientModel::instance()->updatePatient(DrKonqi::pid());
}

int Application::handleNotify(const QCommandLineParser &parser)
{
    setMetadata(parser);

    if (!DrKonqi::init()) {
        return 1;
    }

    //  Whether the user should be encouraged to file a bug report
    const bool interactionAllowed = Settings::interactionAllowed();

    if (isShuttingDown() && !parser.isSet(dialogOption)) {
        return 0;
    }

    // if no notification service is running (eg. shell crashed, or other desktop environment)
    // and we didn't auto-restart the app, open DrKonqi dialog instead of showing an SNI
    // and emitting a desktop notification.
    if ((!StatusNotifier::notificationServiceRegistered() && !parser.isSet(restartedOption)) || parser.isSet(dialogOption)) {
        PatientModel::instance()->setPatient(DrKonqi::pid());
        if (m_engine) {
            raiseWindow();
        } else {
            return !loadUi();
        }
    } else { // StatusNotifierItem (interaction) or notification (no interaction)
        requestDrKonqiDialog(interactionAllowed);
    }

    return 0;
}

[[nodiscard]] std::unique_ptr<QCommandLineParser> commandLineParser()
{
    auto parser = std::make_unique<QCommandLineParser>();

    parser->addOptions({signalOption,
                        appNameOption,
                        appPathOption,
                        appVersionOption,
                        bugAddressOption,
                        programNameOption,
                        productNameOption,
                        pidOption,
                        startupIdOption,
                        saferOption,
                        keepRunningOption,
                        threadOption,
                        dialogOption,
                        glRendererOption,
                        exceptionNameOption,
                        exceptionWhatOption,
                        qtVersionOption,
                        frameworksVersionOption,
                        metadataOption});

    parser->addOption(restartedOption);

    parser->addOption(notify);

    return parser;
}

int main(int argc, char *argv[])
{
    setenv("DRKONQI_BACKEND", "COREDUMPD", 1);
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("drkonqi-coredump-gui"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug"), app.windowIcon()));

    KAboutData aboutData(QStringLiteral("drkonqi"),
                         i18nc("@title CLI title", "Crashed Processes Viewer"),
                         QString::fromLatin1(PROJECT_VERSION),
                         i18nc("@info program description", "Offers detailed view of past crashes"),
                         KAboutLicense::GPL,
                         i18n("(C) 2020-2022, The DrKonqi Authors"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.drkonqi.coredump.gui"));
    KAboutData::setApplicationData(aboutData);

    auto parser = commandLineParser();

    aboutData.setupCommandLine(parser.get());

    // Add all unknown options but make sure to print a warning.
    // This enables older DrKonqi's to run by newer KCrash instances with
    // possibly different/new options.
    // KCrash can always send all options it knows to send and be sure that
    // DrKonqi will not explode on them. If an option is not known here it's
    // either too old or too new.
    //
    // To implement this smartly we'll ::parse all arguments, and then ::process
    // them again once we have injected no-op options for all unknown ones.
    // This allows ::process to still do common argument handling for --version
    // as well as standard error handling.
    if (!parser->parse(app.arguments())) {
        const QStringList unknownOptionNames = parser->unknownOptionNames();
        for (const QString &option : unknownOptionNames) {
            qWarning() << "Unknown option" << option << " - ignoring it.";
            parser->addOption(QCommandLineOption(option));
        }
    }

    parser->process(app);

    DrKonqi::setMetadataFile(parser->value(metadataOption));

    aboutData.processCommandLine(parser.get());

    Application application;

    KDBusService service(KDBusService::Unique);
    service.connect(&service, &KDBusService::activateRequested, &service, [&application](const QStringList &arguments, const QString &) {
        const auto parser = commandLineParser();

        // See comment above
        if (!parser->parse(arguments)) {
            const auto unknownOptionNames = parser->unknownOptionNames();
            for (const auto &option : unknownOptionNames) {
                qWarning() << "Unknown option" << option << " - ignoring it.";
                parser->addOption(QCommandLineOption(option));
            }
        }

        parser->process(arguments);
        if (parser->isSet(notify)) {
            const auto status = application.handleNotify(*parser);

            if (status != 0) {
                exit(status);
            }
        } else {
            application.raiseWindow();
        }
    });

    if (parser->isSet(notify)) {
        const auto status = application.handleNotify(*parser);

        if (status != 0) {
            return status;
        }
    } else {
        if (!application.loadUi()) {
            return -1;
        }
    }

    return app.exec();
}
