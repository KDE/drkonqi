// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.Page {
    globalToolBarStyle: actions.size > 0 ? undefined : Kirigami.ApplicationHeaderStyle.None

    actions: [
        Kirigami.Action {
            icon.name: "system-reboot-symbolic"
            text: i18nc("@action %1 is an application name e.g. kwrite", "Restart %1", CrashedApplication.name)
            visible: !CrashedApplication.hasBeenRestarted
            onTriggered: CrashedApplication.restart()
        }
    ]

    ColumnLayout {
        anchors.fill: parent

        Item {
            Layout.fillHeight: true
        }

        QQC2.ProgressBar {
            id: progressBar
            Layout.alignment: Qt.AlignHCenter
            indeterminate: true
            visible: !reportInterface.crashEventSent
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: !progressBar.visible
            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                source: "data-success"
                width: Kirigami.Units.iconSizes.enormous
                height: width
            }
            QQC2.Label {
                text: i18nc("@label", "Crash Report Sent")
            }
        }

        QQC2.CheckBox {
            Layout.alignment: Qt.AlignHCenter
            checked: Settings.sentry
            text: i18nc("@label", "Always report crashes automatically in the future")
            onToggled: {
                Settings.sentry = checked
                Settings.save()
            }

            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.text: i18nc("@info:tooltip",
`Always automatically submit a crash report to KDE's crash tracking system. No manual input required.
You will not receive any more crash notifications.`)
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
