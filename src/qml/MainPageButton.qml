// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

import org.kde.drkonqi

QQC2.Button {
    Layout.alignment: Qt.AlignHCenter
    icon.width: Kirigami.Units.iconSizes.medium
    icon.height: icon.width
    visible: action.visible
}
