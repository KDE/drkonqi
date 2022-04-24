// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

RowLayout {
    property bool failed: false
    property int usefulness: BacktraceParser.InvalidUsefulness
    property int stars: {
        switch (usefulness) {
        case BacktraceParser.ReallyUseful:
            return 3
        case BacktraceParser.MayBeUseful:
            return 2
        case BacktraceParser.ProbablyUseless:
            return 1
        case BacktraceParser.Useless:
        case BacktraceParser.InvalidUsefulness:
            return 0
        }
    }

    QQC2.Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: {
            switch (usefulness) {
            case BacktraceParser.InvalidUsefulness:
                return i18nc("@info", "Waiting for data…")
            case BacktraceParser.ReallyUseful:
                return i18nc("@info", "The generated crash information is useful");
            case BacktraceParser.MayBeUseful:
                return i18nc("@info", "The generated crash information may be useful");
            case BacktraceParser.ProbablyUseless:
                return i18nc("@info", "The generated crash information is probably not useful");
            case BacktraceParser.Useless:
                return i18nc("@info", "The generated crash information is not useful");
            }
        }
    }

    Kirigami.Icon {
        implicitWidth: Kirigami.Units.iconSizes.small
        implicitHeight: implicitWidth
        source: "data-error"
        visible: failed || stars <= 0
    }

    Kirigami.Icon {
        implicitWidth: Kirigami.Units.iconSizes.small
        implicitHeight: implicitWidth
        source: "rating"
        enabled: stars >= 1
    }

    Kirigami.Icon {
        implicitWidth: Kirigami.Units.iconSizes.small
        implicitHeight: implicitWidth
        source: "rating"
        enabled: stars >= 2
    }

    Kirigami.Icon {
        implicitWidth: Kirigami.Units.iconSizes.small
        implicitHeight: implicitWidth
        source: "rating"
        enabled: stars >= 3
    }

    onUsefulnessChanged: {
        console.log("usefulness")
        console.log(usefulness)
        console.log(BacktraceParser.ReallyUseful)
    }
}
