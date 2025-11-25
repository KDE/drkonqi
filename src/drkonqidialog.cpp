// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "drkonqidialog.h"

#include <KLocalizedString>
#include <KWindowConfig>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "backtracegenerator.h"
#include "backtraceparser.h"
#include "backtracewidget.h"
#include "bugzillaintegration/bugzillalib.h"
#include "config-drkonqi.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqiwidgetsdialog.h"
#include "qmlextensions/credentialstore.h"
#include "qmlextensions/doctore.h"
#include "qmlextensions/platformmodel.h"
#include "qmlextensions/reproducibilitymodel.h"
#include "settings.h"

void DrKonqiDialog::show(DrKonqiDialog::GoTo to)
{
    if (DrKonqi::isSafer() || DrKonqi::minimalMode()) {
        (new DrKonqiWidgetsDialog(this))->show();
        return;
    }

    auto engine = new QQmlApplicationEngine(this);

    static auto l10nContext = new KLocalizedContext(engine);
    l10nContext->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine->rootContext()->setContextObject(l10nContext);

    qmlRegisterType<BugzillaManager>("org.kde.drkonqi", 1, 0, "Bugzilla");
    qmlRegisterType<PlatformModel>("org.kde.drkonqi", 1, 0, "PlatformModel");
    qmlRegisterType<ReproducibilityModel>("org.kde.drkonqi", 1, 0, "ReproducibilityModel");
    qmlRegisterType<CredentialStore>("org.kde.drkonqi", 1, 0, "CredentialStore");
    qmlRegisterType<DebugPackageInstaller>("org.kde.drkonqi", 1, 0, "DebugPackageInstaller");

    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "ReportInterface", ReportInterface::self());
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "CrashedApplication", DrKonqi::crashedApplication());
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "BacktraceGenerator", DrKonqi::debuggerManager()->backtraceGenerator());

    static Doctore doctore;
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "DrKonqi", &doctore);

    auto settings = Settings::self();
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "Settings", settings);

    // TODO do we need this second BG?
    qmlRegisterUncreatableType<BacktraceGenerator>("org.kde.drkonqi", 1, 0, "BacktraceGenerator1", QStringLiteral("Cannot create WarningLevel in QML"));
    qmlRegisterUncreatableType<BacktraceParser>("org.kde.drkonqi", 1, 0, "BacktraceParser", QStringLiteral("Cannot create WarningLevel in QML"));

    const QUrl mainUrl(QStringLiteral("qrc:/ui/main.qml"));
    QObject::connect(
        engine,
        &QQmlApplicationEngine::objectCreated,
        this,
        [mainUrl, this, to](QObject *obj, const QUrl &objUrl) {
            if (!obj && mainUrl == objUrl) {
                qWarning() << "Failed to load QML dialog, falling back to QWidget.";
                (new DrKonqiWidgetsDialog(this))->show();
                return;
            }

            switch (to) {
            case GoTo::Main:
                break;
            case GoTo::Sentry:
                QMetaObject::invokeMethod(obj, "goToSentry", Qt::QueuedConnection);
                break;
            }
        },
        Qt::QueuedConnection);
    engine->load(mainUrl);
}
