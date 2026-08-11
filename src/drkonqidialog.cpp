// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "drkonqidialog.h"

#include <KLocalizedString>
#include <KWindowConfig>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "backtracegenerator.h"
#include "bugzillaintegration/bugzillalib.h"
#include "config-drkonqi.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "debugpackageinstaller.h"
#include "drkonqi.h"
#include "parser/backtraceparser.h"
#include "qmlextensions/credentialstore.h"
#include "qmlextensions/doctore.h"
#include "qmlextensions/platformmodel.h"
#include "qmlextensions/reproducibilitymodel.h"
#include "settings.h"

void DrKonqiDialog::show(DrKonqiDialog::GoTo to)
{
    auto engine = new QQmlApplicationEngine(this);

    static auto l10nContext = new KLocalizedContext(engine);
    l10nContext->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine->rootContext()->setContextObject(l10nContext);

    QObject::connect(
        engine,
        &QQmlApplicationEngine::objectCreated,
        this,
        [to](QObject *obj, const QUrl &) {
            switch (to) {
            case GoTo::Main:
                break;
            case GoTo::Sentry:
                QMetaObject::invokeMethod(obj, "goToSentry", Qt::QueuedConnection);
                break;
            }
        },
        Qt::QueuedConnection);
    engine->loadFromModule("org.kde.drkonqi", "Main");
}

#include "moc_drkonqidialog.cpp"
