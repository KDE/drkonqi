// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

import org.kde.drkonqi as DrKonqi

QQC2.CheckBox {
    Layout.alignment: Qt.AlignHCenter
    checked: DrKonqi.Settings.downloadSymbols
    text: i18nc("@label", "Always automatically enhance crash reports by downloading additional resources in the future")
    onToggled: {
        DrKonqi.Settings.downloadSymbols = checked
        DrKonqi.Settings.save()
    }

    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    QQC2.ToolTip.visible: hovered
    QQC2.ToolTip.text: i18nc("@info:tooltip",
`Crash reports can be of greater value if additional debugging resources are downloaded from your distributor first.
This causes downloads of unknown size when a crash occurs. When using a metered connection this is skipped automatically.`)
}
