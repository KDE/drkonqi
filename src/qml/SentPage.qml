// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.ScrollablePage {
    title: i18nc("@title", "Crash Report Sent")

    property int bugNumber: 0

    actions: [
        Kirigami.Action {
            icon.name: "system-reboot-symbolic"
            text: i18nc("@action %1 is an application name e.g. kwrite", "Restart %1", CrashedApplication.name)
            visible: !CrashedApplication.hasBeenRestarted
            onTriggered: CrashedApplication.restart()
        }
    ]

    ColumnLayout {
        QQC2.Label {
            text: xi18nc("@info", "URL: <link url='%1'>%1</link>", Globals.bugUrl(bugNumber))
            wrapMode: Text.Wrap
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        QQC2.Label {
            text: i18nc("@info", "Thank you for being a part of KDE. You may now close this window.")
            wrapMode: Text.Wrap
        }
    }
}
