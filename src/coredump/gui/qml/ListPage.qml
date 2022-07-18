// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels

import org.kde.drkonqi.coredump.gui 1.0 as DrKonqi

Kirigami.ScrollablePage {
    id: page
    title: i18nc("@title", "Crashes")

    header: QQC2.ToolBar {
        visible: page.state !== "loading" && page.state !== "noData"
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        contentItem: Kirigami.SearchField {
            onAccepted: patientFilterModel.filterString = text
        }
    }

    ListView {
        id: view
        reuseItems: true // We have a lot of items potentially, recycle them

        KItemModels.KSortFilterProxyModel { // set as model during state change
            id: patientFilterModel
            sourceModel: DrKonqi.PatientModel
            filterRole: "ROLE_appName"
            sortRole: "modelIndex"
            sortOrder: Qt.DescendingOrder
        }

        delegate: Kirigami.BasicListItem {
            label: modelObject.appName
            subtitle: modelObject.dateTime
            icon: modelObject.iconName
            onClicked: pageStack.push("qrc:/DetailsPage.qml", {patient: modelObject})
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