// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami

import org.kde.drkonqi 1.0

Kirigami.Page {
    property bool canReport: false

    title: CrashedApplication.name

    Component.onCompleted: {
        if (BacktraceGenerator.state === BacktraceGenerator.NotLoaded) {
            BacktraceGenerator.start()
        }
    }
    Component.onDestruction: Settings.save()

    actions: [
        Kirigami.Action {
            enabled: Kirigami.Settings.isMobile ? true : canReport
            visible: Kirigami.Settings.isMobile ? canReport : true
            icon.name: "tools-report-bug"
            text: i18nc("@action", "Report Bug")
            // TODO: could give context on why the button is disabled when canReport is false
            tooltip: i18nc("@info:tooltip", "Starts the bug report assistant.")
            onTriggered: pageStack.push("qrc:/ui/WelcomePage.qml")
        },
        Kirigami.Action {
            icon.name: "system-reboot"
            text: i18nc("@action", "Restart Application")
            visible: !CrashedApplication.hasBeenRestarted
            onTriggered: CrashedApplication.restart()
        },
        Kirigami.Action {
            icon.name: "code-class"
            text: i18nc("@action", "Developer Information")
            onTriggered: pageStack.push("qrc:/ui/DeveloperPage.qml")
        }
    ]

    ColumnLayout {
        anchors.fill: parent

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: xi18nc("@info", "<para>We are sorry, <application>%1</application> closed unexpectedly.</para>", CrashedApplication.name)
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            onLinkActivated: link => Qt.openUrlExternally(link)
            text: {
                canReport = false

                if (CrashedApplication.bugReportAddress.length <= 0) {
                    return xi18nc("@info",
                        '<para>You cannot report this error, because <application>%1</application> does not provide a bug reporting address.</para>',
                        CrashedApplication.name);
                }

                if (CrashedApplication.fakeExecutableBaseName === "drkonqi") {
                    return xi18nc("@info",
`<para>As the Crash Handler itself has failed, the
automatic reporting process is disabled to reduce the
risks of failing again.<nl /><nl />
Please, <link url='%1'>manually report</link> this error
to the KDE bug tracking system. Do not forget to include
the backtrace from the <interface>Developer Information</interface>
tab.</para>`,
                                Globals.ownBugzillaUrl);
                }

                if (DrKonqi.isSafer()) {
                    return xi18nc("@info",
`<para>The reporting assistant is disabled because the crash handler dialog was started in safe mode.<nl />
You can manually report this bug to <link>%1</link> (including the backtrace from the <interface>Developer Information</interface> tab.)</para>`,
                                CrashedApplication.bugReportAddress);
                }

                if (CrashedApplication.hasDeletedFiles) {
                    return xi18nc("@info",
`<para>The reporting assistant is disabled because
the crashed application appears to have been updated or
uninstalled since it had been started. This prevents accurate
crash reporting and can also be the cause of this crash.</para>
<para>After updating it is always a good idea to log out and back
in to make sure the update is fully applied and will not cause
any side effects.</para>`);
                }

                canReport = true
                return xi18nc("@info",
                              "<para>You can help us improve KDE Software by reporting this error.<nl /><link url='%1'>Learn more about bug reporting.</link></para>",
                               Globals.aboutBugReportingUrl);
            }
        }

        QQC2.CheckBox {
            checked: Settings.sentry
            text: i18nc("@label", "Automatically submit crash data")
            onToggled: {
                Settings.sentry = checked
                Settings.save()
            }
        }

        Item {
            Layout.fillHeight: true
        }

        ColumnLayout {
            spacing: 0

            QQC2.Label {
                font.bold: true
                text: i18nc("@label", "Details:")
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: xi18nc("@info Note the time information is divided into date and time parts",
                            '<para>Executable: <application>%1</application> PID: %2 Signal: %3 (%4) Time: %5</para>',
                                CrashedApplication.fakeExecutableBaseName,
                                CrashedApplication.pid,
                                CrashedApplication.signalName,
                                CrashedApplication.signalNumber,
                                Qt.formatDateTime(CrashedApplication.datetime))
            }
        }
    }
}
