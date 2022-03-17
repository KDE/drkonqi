// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.ScrollablePage {
    id: page

    title: i18nc("@title", "Problem is Already Reported")

    property var id: null
    property var duplicateOf: null
    property var alreadyClosed: null

    ColumnLayout {
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: {
                const bugNumber = String(reportInterface.duplicateId)
                const bugUrl = Globals.bugUrl(bugNumber)
                if (alreadyClosed) {
                    if (duplicateOf !== null) {
                        return xi18nc("@info", "Your crash has already been reported as <link url=\"%1\">Bug %1</link>, which is a duplicate of the <emphasis strong='true'>closed</emphasis> <link url=\"%2\">Bug %2</link>.",
                        bugUrl, bugNumber,
                        Globals.bugUrl(duplicateOf), String(duplicateOf)
                        )
                    }
                    return xi18nc("@info", "Your crash has already been reported as <link url=\"%1\">Bug %1</link> which has been <emphasis strong='true'>closed</emphasis>.",
                    bugUrl, bugNumber)
                }

                if (duplicateOf !== null) {
                    return xi18nc("@info", "Your crash has already been reported as <link url=\"%1\">Bug %1</link>, which is a <emphasis strong='true'>duplicate</emphasis> of <link url=\"%2\">Bug %2</link>",
                    bugUrl, bugNumber,
                    Globals.bugUrl(duplicateOf), String(duplicateOf))
                }
                return xi18nc("@info", "Your crash is a <emphasis strong='true'>duplicate</emphasis> and has already been reported as <link url=\"%1\">Bug %2</link>.",
                bugUrl, bugNumber)
            }
            onLinkActivated: Qt.openUrlExternally(link)
        }
        QQC2.Label {
            visible: alreadyClosed !== null
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: xi18nc("@label", "You may choose to add additional information, but you should only do so if you have new or requested information.");
        }
    }

    footer: FooterActionBar {
        actions: [
            Kirigami.Action {
                // FIXME is there a kstandardguiitem thing for qml?
                text: i18nc("@action:button", "Close")
                onTriggered: Qt.quit()
            },
            Kirigami.Action {
                iconName: "mail-attachment"
                text: i18nc("@action:button", "Attach Additional Information")
                onTriggered: {
                    reportInterface.attachToBugNumber = reportInterface.duplicateId
                    pageStack.push('qrc:/ui/ReportPage.qml')
                }
            }
        ]
    }
}
