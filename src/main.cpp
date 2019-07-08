/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#include <cstdlib>
#include <unistd.h>

#include <QIcon>

#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QDebug>

#include <config-drkonqi.h>
#if HAVE_X11
#include <QX11Info>
#endif
#ifdef Q_OS_MACOS
#include <KWindowSystem>
#endif

#include "config-drkonqi.h"
#include "drkonqi.h"
#include "drkonqidialog.h"
#include "statusnotifier.h"

static const char version[] = PROJECT_VERSION;
static const char description[] = I18N_NOOP("The KDE Crash Handler gives the user feedback "
                                            "if a program has crashed.");

int main(int argc, char* argv[])
{
#ifndef Q_OS_WIN //krazy:exclude=cpp
// Drop privs.
    setgid(getgid());
    if (setuid(getuid()) < 0 && geteuid() != getuid()) {
        exit(255);
    }
#endif

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    KLocalizedString::setApplicationDomain("drkonqi5");

    // Prevent KApplication from setting the crash handler. We will set it later...
    setenv("KDE_DEBUG", "true", 1);
    // Session management is not needed, do not even connect in order to survive longer than ksmserver.
    unsetenv("SESSION_MANAGER");

    KAboutData aboutData(QStringLiteral("drkonqi"), i18n("The KDE Crash Handler"),
                         QString::fromLatin1(version), i18n(description),
                         KAboutLicense::GPL,
                         i18n("(C) 2000-2018, The DrKonqi Authors"));
    aboutData.addAuthor(i18nc("@info:credit","Hans Petter Bieker"), QString(),
                         QStringLiteral("bieker@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit","Dario Andres Rodriguez"), QString(),
                         QStringLiteral("andresbajotierra@gmail.com"));
    aboutData.addAuthor(i18nc("@info:credit","George Kiagiadakis"), QString(),
                         QStringLiteral("gkiagia@users.sourceforge.net"));
    aboutData.addAuthor(i18nc("@info:credit","A. L. Spehr"), QString(),
                         QStringLiteral("spehr@kde.org"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug"), app.windowIcon()));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    const QCommandLineOption signalOption(QStringLiteral("signal"), i18nc("@info:shell","The signal <number> that was caught"), QStringLiteral("number"));
    const QCommandLineOption appNameOption(QStringLiteral("appname"), i18nc("@info:shell","<Name> of the program"), QStringLiteral("name"));
    const QCommandLineOption appPathOption(QStringLiteral("apppath"), i18nc("@info:shell","<Path> to the executable"), QStringLiteral("path"));
    const QCommandLineOption appVersionOption(QStringLiteral("appversion"), i18nc("@info:shell","The <version> of the program"), QStringLiteral("version"));
    const QCommandLineOption bugAddressOption(QStringLiteral("bugaddress"), i18nc("@info:shell","The bug <address> to use"), QStringLiteral("address"));
    const QCommandLineOption programNameOption(QStringLiteral("programname"), i18nc("@info:shell","Translated <name> of the program"), QStringLiteral("name"));
    const QCommandLineOption pidOption(QStringLiteral("pid"), i18nc("@info:shell","The <PID> of the program"), QStringLiteral("pid"));
    const QCommandLineOption startupIdOption(QStringLiteral("startupid"), i18nc("@info:shell","Startup <ID> of the program"), QStringLiteral("id"));
    const QCommandLineOption kdeinitOption(QStringLiteral("kdeinit"), i18nc("@info:shell","The program was started by kdeinit"));
    const QCommandLineOption saferOption(QStringLiteral("safer"), i18nc("@info:shell","Disable arbitrary disk access"));
    const QCommandLineOption restartedOption(QStringLiteral("restarted"), i18nc("@info:shell","The program has already been restarted"));
    const QCommandLineOption keepRunningOption(QStringLiteral("keeprunning"), i18nc("@info:shell","Keep the program running and generate "
                                                    "the backtrace at startup"));
    const QCommandLineOption threadOption(QStringLiteral("thread"), i18nc("@info:shell","The <thread id> of the failing thread"), QStringLiteral("threadid"));
    const QCommandLineOption dialogOption(QStringLiteral("dialog"), i18nc("@info:shell","Do not show a notification but launch the debug dialog directly"));

    parser.addOptions({
        signalOption,
        appNameOption,
        appPathOption,
        appVersionOption,
        bugAddressOption,
        programNameOption,
        pidOption,
        startupIdOption,
        kdeinitOption,
        saferOption,
        restartedOption,
        keepRunningOption,
        threadOption,
        dialogOption
    });

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
    if (!parser.parse(app.arguments())) {
        for (const QString &option : parser.unknownOptionNames()) {
            qWarning() << "Unknown option" << option << " - ignoring it.";
            parser.addOption(QCommandLineOption(option));
        }
    }

    parser.process(app);
    aboutData.processCommandLine(&parser);

    DrKonqi::setSignal(parser.value(signalOption).toInt());
    DrKonqi::setAppName(parser.value(appNameOption));
    DrKonqi::setAppPath(parser.value(appPathOption));
    DrKonqi::setAppVersion(parser.value(appVersionOption));
    DrKonqi::setBugAddress(parser.value(bugAddressOption));
    DrKonqi::setProgramName(parser.value(programNameOption));
    DrKonqi::setPid(parser.value(pidOption).toInt());
    DrKonqi::setKdeinit(parser.isSet(kdeinitOption));
    DrKonqi::setSafer(parser.isSet(saferOption));
    DrKonqi::setRestarted(parser.isSet(restartedOption));
    DrKonqi::setKeepRunning(parser.isSet(keepRunningOption));
    DrKonqi::setThread(parser.value(threadOption).toInt());
    auto forceDialog = parser.isSet(dialogOption);

#if HAVE_X11
    const QString startupId = parser.value(startupIdOption);
    if (!startupId.isEmpty()) {
        QX11Info::setNextStartupId(startupId.toUtf8());
    }
#endif

    if (!DrKonqi::init()) {
        return 1;
    }

    app.setQuitOnLastWindowClosed(false);

    auto openDrKonqiDialog = [&app]{
        DrKonqiDialog *w = new DrKonqiDialog();
        QObject::connect(w, &DrKonqiDialog::rejected, &app, &QApplication::quit);
        w->show();
#ifdef Q_OS_MACOS
        KWindowSystem::forceActiveWindow(w->winId());
#endif
    };

    bool restarted = parser.isSet(restartedOption);

    // if no notification service is running (eg. shell crashed, or other desktop environment)
    // and we didn't auto-restart the app, open DrKonqi dialog instead of showing an SNI
    // and emitting a desktop notification
    if (forceDialog || (!restarted && !StatusNotifier::notificationServiceRegistered())) {
        openDrKonqiDialog();
    } else {
        StatusNotifier *statusNotifier = new StatusNotifier();
        if (!restarted) {
            statusNotifier->notify();
        }
        QObject::connect(statusNotifier, &StatusNotifier::expired, &app, &QApplication::quit);
        QObject::connect(statusNotifier, &StatusNotifier::activated, openDrKonqiDialog);
    }

    int ret = app.exec();

    return ret;
}
