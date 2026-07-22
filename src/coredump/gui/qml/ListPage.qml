// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

import org.kde.drkonqi.coredump.gui as DrKonqi

Kirigami.ScrollablePage {
    id: page
    title: i18nc("@title", "Crashes")

    actions: [
        Kirigami.Action {
            displayComponent: Kirigami.SearchField {
                onAccepted: patientFilterModel.filterString = text
            }
        }
    ]

    ListView {
        id: view

        property bool argumentsApplied: Application.arguments.length > 1 ? false : true

        reuseItems: true // We have a lot of items potentially, recycle them

        KItemModels.KSortFilterProxyModel { // set as model during state change
            id: patientFilterModel
            sourceModel: DrKonqi.PatientModel
            filterRoleName: "ROLE_appName"
            sortRoleName: "modelIndex"
            sortOrder: Qt.DescendingOrder
        }

        onCountChanged: {
            if (argumentsApplied || !model) {
                return
            }
            for (let i = 0; i < model.count; ++i) {
                const patient = model.data(model.index(i, 0), DrKonqi.PatientModel.ObjectRole)
                if (patient.journalCursor === Application.arguments[1]) {
                    view.currentIndex = i
                    view.currentItem.clicked()

                    argumentsApplied = true
                    return
                }
            }
        }

        delegate: QQC2.ItemDelegate {
            id: delegate

            required property int modelIndex
            required property var modelObject

            text: modelObject.appName
            icon.name: modelObject.iconName

            highlighted: modelIndex === DrKonqi.PatientModel.currentIndex

            width: ListView.view.width
            onClicked: DrKonqi.PatientModel.currentIndex = modelIndex

            contentItem: Kirigami.IconTitleSubtitle {
                title: delegate.text
                subtitle: delegate.modelObject.dateTime
                icon: icon.fromControlsIcon(delegate.icon)
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: page.state === "loading"
            icon.name: "search"
            text: i18nc("@info place holder for empty listview", "Loading crash reports")
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: page.state === "noData"
            icon.name: "emblem-checked"
            text: i18nc("@info place holder for empty listview", "No processes have crashed yet")
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: page.state === "badSearch"
            icon.name: "search"
            text: i18nc("@info place holder for empty listview", "No crashes matching the search")
        }
    }

    states: [
        State {
            name: "loading"
            when: !DrKonqi.PatientModel.ready
        },
        State {
            name: "noData"
            when: DrKonqi.PatientModel.count === 0
        },
        State {
            name: "badSearch"
            when: patientFilterModel.count === 0
        },
        State {
            name: "" // default state
            PropertyChanges { target: view; model: patientFilterModel }
        }
    ]
}
