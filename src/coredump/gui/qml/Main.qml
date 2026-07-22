// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import org.kde.config as KConfig
import org.kde.kirigami as Kirigami

import org.kde.drkonqi.coredump.gui

Kirigami.ApplicationWindow {
    id: root

    title: i18nc("@title:window", "Overview")
    minimumWidth: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
    width: Kirigami.Settings.isMobile ? undefined : Kirigami.Units.gridUnit * 42
    height: Kirigami.Settings.isMobile ? undefined : Kirigami.Units.gridUnit * 34

    KConfig.WindowStateSaver {
        configGroupName: "MainWindow"
    }

    readonly property Item applicationStates : Item {
        states: [
            State {
                when: PatientModel.currentIndex === -1
                PropertyChanges {
                    root.pageStack.items: [listPage]
                }
            },
            State {
                when: PatientModel.currentIndex !== -1
                PropertyChanges {
                    root.pageStack.items: [listPage, detailsPage]
                }
            }
        ]
    }

    readonly property ListPage listPage : ListPage {
        parent: root.applicationStates
    }

    readonly property DetailsPage detailsPage : DetailsPage {
        parent: root.applicationStates

        patient: PatientModel.currentPatient
    }

    pageStack.initialPage: ListPage {}
}
