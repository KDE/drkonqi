// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi.coredump.gui 1.0 as DrKonqi

Kirigami.ApplicationWindow {
    id: root

    title: i18nc("@title:window", "Intensive Care")
    minimumWidth: Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Units.gridUnit * 22

    pageStack.initialPage: ListPage {}
    pageStack.defaultColumnWidth: root.width // show single page
}
