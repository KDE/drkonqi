// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Item {
    property alias leftActions: _toolbarLeft.actions
    property alias rightActions: _toolbarRight.actions
    property alias actions: _toolbarRight.actions

    height: Math.max(_toolbarLeft.height, _toolbarRight.height) + Kirigami.Units.largeSpacing * 2

    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Header

    Rectangle {
         color: Kirigami.Theme.backgroundColor
         anchors.fill: parent

         Kirigami.ActionToolBar {
             id: _toolbarLeft

             anchors.bottom: parent.bottom
             anchors.bottomMargin: Kirigami.Units.largeSpacing
             anchors.left: parent.left
             anchors.leftMargin: Kirigami.Units.largeSpacing

             alignment: Qt.AlignLeft
             flat: false
         }

         Kirigami.ActionToolBar {
             id: _toolbarRight

             anchors.bottom: parent.bottom
             anchors.bottomMargin: Kirigami.Units.largeSpacing
             anchors.right: parent.right
             anchors.rightMargin: Kirigami.Units.largeSpacing

             alignment: Qt.AlignRight
             flat: false
         }

         Kirigami.Separator {
             id: footerSeparator

             anchors.bottom: parent.top
             width: parent.width
         }
    }
}
