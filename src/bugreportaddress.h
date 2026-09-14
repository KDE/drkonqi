/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BUGREPORTADDRESS_H
#define BUGREPORTADDRESS_H

#include <QString>
#include <QUrl>

#include "drkonqi_debug.h"
#include "drkonqi_globals.h"

class BugReportAddress : public QString
{
public:
    BugReportAddress()
        : QString()
    {
    }
    BugReportAddress(const QString &address)
        : QString(looksLikeKDE(address) ? KDE_BUGZILLA_URL : address)
    {
        if (!isKdeBugzilla()) {
            qCWarning(DRKONQI_LOG) << "This application doesn't seem to be from KDE. Refusing to report to KDE infrastructure!";
        }
    }

    bool isKdeBugzilla() const
    {
        return *this == KDE_BUGZILLA_URL;
    }

    bool isEmail() const
    {
        return contains(QLatin1Char('@'));
    }

private:
    [[nodiscard]] static bool looksLikeKDE(const QString &address)
    {
        // The default value is a raw email address from the days of yore. Special case it since it is not a URI.
        if (address == QLatin1String("submit@bugs.kde.org")) {
            return true;
        }

        // In all other cases we expect a URI and check that it points to something vaguely KDE.
        QString host = [&] {
            QUrl url(address, QUrl::StrictMode);
            if (!url.isValid()) {
                qCWarning(DRKONQI_LOG) << "Invalid URI" << address;
                return QString();
            }
            // URIs (e.g. mailto:) will parse into scheme and path only. Path will also parse fragments though so it should be clean™
            return url.host().isEmpty() ? url.path() : url.host();
        }();
        // Cover all bases here: bugs.kde.org, bugstest.kde.org, invent.kde.org...
        return host.endsWith(QLatin1String(".kde.org"));
    }
};

#endif
