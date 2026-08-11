// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <KAboutData>
#include <KLocalizedString>

#include <config-drkonqi.h>

#include "DetailsLoader.h"
#include "Patient.h"
#include "PatientModel.h"

int main(int argc, char *argv[])
{
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

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KLocalizedContext i18nContext;
    i18nContext.setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(&i18nContext);

    engine.loadFromModule("org.kde.drkonqi.coredump.gui", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
