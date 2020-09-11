/*******************************************************************
* applicationdetailsexamples.cpp
* SPDX-FileCopyrightText: 2010 Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* SPDX-License-Identifier: GPL-2.0-or-later
*
******************************************************************/

#include "applicationdetailsexamples.h"

#include <KLocalizedString>

#include "drkonqi.h"
#include "crashedapplication.h"

ApplicationDetailsExamples::ApplicationDetailsExamples(QObject * parent)
    : QObject(parent)
{
    QString binaryName = DrKonqi::crashedApplication()->fakeExecutableBaseName();

    if (binaryName == QLatin1String("plasmashell")) {
       m_examples = i18nc("@info examples about information the user can provide",
       "Widgets you have in your desktop and panels (both official and unofficial), "
       "desktop settings (wallpaper plugin, themes) and activities.");
    } else if (binaryName == QLatin1String("kwin_x11") || binaryName == QLatin1String("kwin_wayland")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "State of Desktop Effects (Compositing), kind of effects enabled, window decoration, "
        "and specific window rules and configuration.");
    } else if (binaryName == QLatin1String("konqueror") ||
        binaryName == QLatin1String("rekonq")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "sites you were visiting, number of opened tabs, plugins you have installed, "
        "and any other non-default setting.");
    } else if (binaryName == QLatin1String("dolphin")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "File view mode, grouping and sorting settings, preview settings, and directory you were browsing.");
    } else if (binaryName == QLatin1String("kopete")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "Instant Messaging protocols you use, and plugins you have installed (official and unofficial).");
    } else if (binaryName == QLatin1String("kmail")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "Mail protocols and account-types you use.");
    } else if (binaryName == QLatin1String("kwrite") ||
        binaryName == QLatin1String("kate") ||
        binaryName == QLatin1String("kword")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "Type of the document you were editing.");
    } else if (binaryName == QLatin1String("juk") ||
        binaryName == QLatin1String("amarok") ||
        binaryName == QLatin1String("dragon") ||
        binaryName == QLatin1String("kaffeine")) {
        m_examples = i18nc("@info examples about information the user can provide",
        "Type of media (extension and format) you were watching and/or listening to.");
    }
}

bool ApplicationDetailsExamples::hasExamples() const
{
    return !m_examples.isEmpty();
}

QString ApplicationDetailsExamples::examples() const
{
    return m_examples;
}
