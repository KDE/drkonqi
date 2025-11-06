// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.Page {
    title: i18nc("@title", "Automatic Report")

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

        QQC2.Label {
            Layout.fillWidth: true
            text: i18nc("@label submitting as crash report", "Collecting crash data. The collected data will automatically be submitted in the background.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            visible: !reportInterface.crashEventSent
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

        Kirigami.FormLayout {
            Layout.alignment: Qt.AlignHCenter

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
            }

            QQC2.CheckBox {
                Kirigami.FormData.label: i18nc("@label other side of row 'in the future: [x] submit stuff automatically", "In the future:")

                checked: Settings.sentry
                text: i18nc("@label", "Automatically report crashes")
                onToggled: {
                    Settings.sentry = checked
                    Settings.save()
                }

                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18nc("@info:tooltip",
`Always automatically submit a crash report to KDE's crash tracking system, with no manual input required.
You will not receive any more crash notifications.`)
            }

            DownloadSymbolsCheckBox {
                Layout.fillWidth: true
            }
        }

        QQC2.ScrollView {
            id: detailView
            Layout.fillWidth: true
            Layout.fillHeight: true

            QQC2.TextArea {
                id: detailArea
                placeholderText: i18nc("@label placeholder text in TextArea", "Tell us more about the crash…")
                wrapMode: Text.Wrap
            }
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignRight
            visible: detailView.visible
            action: Kirigami.Action {
                icon.name: "document-send-symbolic"
                text: i18nc("@action:button", "Send Message")
                onTriggered: {
                    enabled = false
                    // A message is not a coredump but that distinction is lost on users. Let them send message without
                    // actually sending anything if it is likely useless.
                    // https://bugs.kde.org/show_bug.cgi?id=505735
                    if (detailArea.text.length < 4) {
                        return
                    }
                    reportInterface.createCrashMessage(detailArea.text)
                    detailView.visible = false
                }
            }
        }

        Item {
            visible: !detailView.visible
            Layout.fillHeight: true
        }

        QQC2.Button {
            visible: !detailView.visible
            Layout.alignment: Qt.AlignRight
            enabled: reportInterface.crashEventSent
            icon.name: "checkmark-symbolic"
            text: i18nc("@action:button", "Finish")
            onClicked: {
                Qt.quit()
            }
        }
    }
}
