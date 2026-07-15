// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

import org.kde.drkonqi

Kirigami.ScrollablePage {
    id: page

    title: i18nc("@title", "Bug Report Preview")

    ColumnLayout {
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: xi18nc("@label/rich", "<para>This is a preview of the contents of the report which will be sent.</para><para>If you want to modify the report, return to the previous pages.</para>")
        }

        // FIXME height is off maybe put the scrollview on the area instead of scrolling the page?
        QQC2.TextArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: contentHeight
            readOnly: true
            text: reportInterface.generateReportFullText(ReportInterface.DrKonqiStamp.Include, ReportInterface.Backtrace.Complete)
            wrapMode: TextEdit.Wrap

        }
    }

    footer: FooterActionBar {
        actions: [
            Kirigami.Action {
                icon.name: "submit"
                text: i18nc("@action:button", "Submit")
                onTriggered: pageStack.push(Qt.resolvedUrl("SendingPage.qml"))
            }
        ]
    }
}
