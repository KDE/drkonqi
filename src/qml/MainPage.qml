// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

import org.kde.drkonqi

Kirigami.Page {
    title: i18nc("@title", "Crash Handler")

    Component.onCompleted: {
        if (BacktraceGenerator.state === BacktraceGenerator.NotLoaded) {
            BacktraceGenerator.start()
        }
    }
    Component.onDestruction: Settings.save()

    ColumnLayout {
        id: layout

        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        readonly property int widestMainPageButton: Math.max(autoReportButton.implicitWidth,
                                                             devInfoButton.implicitWidth
                                                            )

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            wrapMode: Text.WordWrap
            text: xi18nc("@info", "<application>%1</application> has closed unexpectedly.", CrashedApplication.name)
        }

        Item {
            height: Kirigami.Units.largeSpacing
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            onLinkActivated: link => Qt.openUrlExternally(link)
            text: canReportText
        }

        Item {
            Layout.fillHeight: true
        }

        MainPageButton {
            id: autoReportButton

            Layout.preferredWidth: layout.widestMainPageButton

            action: Kirigami.Action {
                enabled: Kirigami.Settings.isMobile ? true : canAutoReport
                visible: Kirigami.Settings.isMobile ? canAutoReport : true
                icon.name: "document-send-symbolic"
                text: i18nc("@action", "Send Automatic Report")
                onTriggered: {
                    reportInterface.setSendWhenReady(true)
                    reportInterface.sendSentryReport()
                    pageStack.push(Qt.resolvedUrl("SentryPage.qml"))
                }
            }
        }

        MainPageButton {
            id: devInfoButton

            Layout.preferredWidth: layout.widestMainPageButton

            action: Kirigami.Action {
                icon.name: "code-class-symbolic"
                text: i18nc("@action", "See Developer Information")
                onTriggered: pageStack.push(Qt.resolvedUrl("DeveloperPage.qml"))
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
