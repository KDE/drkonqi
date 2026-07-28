// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "drkonqidialog.h"

#include <KLocalizedString>
#include <KWindowConfig>
#include <KWindowSystem>

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

#include "config-drkonqi.h"
#include "drkonqi.h"

void DrKonqiDialog::show(DrKonqiDialog::GoTo to, const QString &windowToken)
{
    auto engine = new QQmlApplicationEngine(this);

    static auto l10nContext = new KLocalizedContext(engine);
    l10nContext->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine->rootContext()->setContextObject(l10nContext);

    QObject::connect(
        engine,
        &QQmlApplicationEngine::objectCreated,
        this,
        [to, windowToken](QObject *obj, const QUrl &) {
            if (auto window = qobject_cast<QWindow *>(obj)) {
                if (!windowToken.isEmpty()) {
                    KWindowSystem::setMainWindow(window, windowToken);
                }
                switch (to) {
                case GoTo::Main:
                    break;
                case GoTo::Sentry:
                    QMetaObject::invokeMethod(obj, "goToSentry", Qt::QueuedConnection);
                    break;
                }
            }
        },
        Qt::QueuedConnection);
    engine->loadFromModule("org.kde.drkonqi", "Main");
}

#include "moc_drkonqidialog.cpp"
