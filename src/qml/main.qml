// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.ApplicationWindow {
    id: appWindow

    property alias bugzilla: reportInterface.bugzilla

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton | Kirigami.ApplicationHeaderStyle.ShowForwardButton

    title: CrashedApplication.name
    minimumWidth: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.gridUnit * 22
    height: minimumHeight

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    ReportInterface {
        id: reportInterface
    }

    DuplicateModel {
        id: duplicateModel
        manager: bugzilla
        iface: reportInterface
    }

    pageStack.initialPage: MainPage {}
    pageStack.defaultColumnWidth: appWindow.width // show single page
}
