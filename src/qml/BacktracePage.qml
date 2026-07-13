// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import org.kde.drkonqi

DeveloperPage {
    title: i18nc("@title:window", "Fetching the Backtrace (Automatic Crash Information)")
    reportActionVisible: false

    onTraceChanged: {
        reportInterface.backtrace = trace
    }

    footerActions: [
        Kirigami.Action {
            enabled: {
                if (DrKonqi.ignoreQuality() && state == BacktraceGenerator.Loaded) {
                    return true;
                }
                switch (usefulness) {
                case BacktraceParser.ReallyUseful:
                case BacktraceParser.MayBeUseful:
                    return true
                case BacktraceParser.ProbablyUseless:
                case BacktraceParser.Useless:
                case BacktraceParser.InvalidUsefulness:
                    return false
                }
            }
            icon.name: "go-next"
            text: i18nc("@action:button", "Next")
            onTriggered: pageStack.push("qrc:/ui/BugzillaPage.qml")
        }
    ]
}
