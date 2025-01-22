// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "doctore.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QUrl>

#include <KOSRelease>

#include "crashedapplication.h"
#include "drkonqi.h"

void Doctore::saveReport(const QString &text)
{
    DrKonqi::saveReport(text, nullptr);
}

void Doctore::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text.trimmed());
}

QString Doctore::appName()
{
    return DrKonqi::crashedApplication()->name();
}

QString Doctore::kdeBugzillaURL()
{
    return DrKonqi::kdeBugzillaURL();
}

QString Doctore::kdeBugzillaDomain()
{
    QUrl bugzillaUrl(DrKonqi::kdeBugzillaURL());
    return bugzillaUrl.host();
}

bool Doctore::isSafer()
{
    return DrKonqi::isSafer();
}

SystemInformation *Doctore::systemInformation()
{
    return DrKonqi::systemInformation();
}

Q_INVOKABLE bool Doctore::ignoreQuality()
{
    return DrKonqi::ignoreQuality();
}

Q_INVOKABLE bool Doctore::isUsingCoredumpd()
{
    return DrKonqi::instance()->isUsingCoredumpd();
}

Q_INVOKABLE QString Doctore::distributionBugReportUrl()
{
    return KOSRelease().bugReportUrl();
}

#include "moc_doctore.cpp"
