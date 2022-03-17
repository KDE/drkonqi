// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.Page {
    id: page

    title: i18nc("@title", "Look for Possible Duplicate Reports")

    property bool hasPerfectDuplicate: reportInterface.duplicateId > 0
    property bool loading: true
    onLoadingChanged: {
        if (loading) {
            return
        }

        if (hasPerfectDuplicate) {
            pageStack.pop()
            pageStack.push("qrc:/ui/PerfectDuplicatePage.qml")
        } else {
            pageStack.pop()
            pageStack.push("qrc:/ui/DuplicatesPage.qml")
        }
    }

    Connections {
        id: connectors
        target: loading ? duplicateModel : null
        onSearchingChanged: {
            if (!duplicateModel.searching) {
                loading = false
            }
        }
        enabled: false // enabled when searching starts
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 8)
        text: i18nc("@info", "Searching bug database for duplicates…")

        QQC2.BusyIndicator {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
        }
    }

    Component.onCompleted: {
        connectors.enabled = true
        duplicateModel.searchBugs(reportInterface.relatedBugzillaProducts(), "crash", reportInterface.firstBacktraceFunctions().join(' '))
    }
}
