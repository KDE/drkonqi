// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.ScrollablePage {
    id: page

    title: i18nc("@title", "Look for Possible Duplicate Reports")

    ColumnLayout {
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: xi18nc("@info/rich", "See if your bug has already been reported. Double click a report in the list and compare it to yours. You can suggest that your crash is a duplicate of that report or directly attach your information to it.")
        }
        ListView {
            id: listView

            property var possibleDuplicates: []
            onPossibleDuplicatesChanged: {
                reportInterface.setPossibleDuplicates(possibleDuplicates)
            }

            implicitHeight: contentHeight
            Layout.fillWidth: true

            model: duplicateModel

            onMovementEnded: {
                if(atYEnd) {
                    console.log("End of list!");
                    // FIXME doesnt work with scrollbar
                    duplicateModel.searchBugs(reportInterface.relatedBugzillaProducts(), "crash", reportInterface.firstBacktraceFunctions().join(' '))
                }
            }

            Kirigami.PromptDialog {
                id: promptDialog
                title: i18nc("@title", "Duplicate?")
                subtitle: i18nc("@label", "Are you quite certain your crash is a duplicate of this bug report?")
                standardButtons: Kirigami.Dialog.Yes | Kirigami.Dialog.No
                property int bugNumber: 0

                onAccepted: {
                    showPassiveNotification("Accepted")
                    reportInterface.attachToBugNumber = bugNumber
                }
                onRejected: console.log("Rejected")
            }

            delegate: Kirigami.SwipeListItem {
                property bool isDuplicate: reportInterface.attachToBugNumber === ROLE_Number

                highlighted: isDuplicate
                onClicked: Qt.openUrlExternally(Globals.bugzillaUrl + "show_bug.cgi?id=" + ROLE_Number)

                contentItem: Kirigami.IconTitleSubtitle {
                    icon.name: reportInterface.attachToBugNumber === ROLE_Number ? "document-duplicate" : ""
                    title: ROLE_Title
                    subtitle: ROLE_Number
                }

                actions: [
                    Kirigami.Action {
                        visible: !isDuplicate
                        text: i18nc("@action:button", "Crash is a duplicate")
                        tooltip: xi18nc("@info:tooltip",
                                     `Use this action when you are certain that your crash is a duplicate of this bug report`)
                        icon.name: "document-duplicate"
                        onTriggered: {
                            removeAction.trigger()
                            promptDialog.bugNumber = ROLE_Number
                            promptDialog.open()
                        }
                        Accessible.name: text
                    },
                    Kirigami.Action {
                        visible: isDuplicate
                        text: i18nc("@action:button", "Crash is not a duplicate")
                        icon.name: "edit-clone-unlink"
                        onTriggered: reportInterface.attachToBugNumber = 0
                    },

                    Kirigami.Action {
                        id: addAction
                        visible: !listView.possibleDuplicates.includes(ROLE_Number) && !isDuplicate
                        text: i18nc("@action:button", "Suggest this crash is related")
                        tooltip: xi18nc("@info:tooltip",
                                     `Use this button to suggest that
                                     the crash you experienced is related to this bug
                                     report`)
                        icon.name: "list-add"
                        onTriggered: {
                            listView.possibleDuplicates.unshift(ROLE_Number)
                            listView.possibleDuplicatesChanged()
                        }
                        Accessible.name: text
                    },
                    Kirigami.Action {
                        id: removeAction
                        visible: !addAction.visible && !isDuplicate
                        text: i18nc("@action:button", "This crash is not related")
                        icon.name: "list-remove"
                        onTriggered: {
                            const index = listView.possibleDuplicates.indexOf(ROLE_Number)
                            if (index > -1) {
                                listView.possibleDuplicates.splice(index, 1)
                                listView.possibleDuplicatesChanged()
                            }
                        }
                        Accessible.name: text
                    }
                ]
            }
        }
    }

    footer: FooterActionBar {
        actions: [
            Kirigami.Action {
                icon.name: "go-next"
                text: i18nc("@action:button", "Next")
                onTriggered: {
                    pageStack.push('qrc:/ui/ReportPage.qml')
                }
            }
        ]
    }
}
