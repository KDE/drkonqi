// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

QQC2.ToolBar {
    property alias actions: _actionToolBar.actions

    height: Kirigami.Units.smallSpacing * 2 + _actionToolBar.height + 1
    position: QQC2.ToolBar.Footer

    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Header

    Kirigami.ActionToolBar {
        id: _actionToolBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        alignment: Qt.AlignRight
        flat: false
    }
}
