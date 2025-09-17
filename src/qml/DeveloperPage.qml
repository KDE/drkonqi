// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.syntaxhighlighting 1.0

import org.kde.drkonqi 1.0

Kirigami.ScrollablePage {
    id: page

    property alias reportActionVisible: reportAction.visible
    property string trace: ""
    property alias usefulness: ratingItem.usefulness
    property alias footerActions: footerBar.actions

    padding: 0
    bottomPadding: 0

    title: i18nc("@title:window", "Developer Information")

    actions: [
        Kirigami.Action {
            id: installButton
            visible: false
            text: i18nc("@action:button", "Install Debug Symbols")
            onTriggered: {
                if (debugPackageInstaller.canInstallDebugPackages) { // prefer the installer when available over dynamic resolution
                    debugPackageInstaller.installDebugPackages()
                } else if (BacktraceGenerator.supportsSymbolResolution) {
                    traceArea.text = ""
                    BacktraceGenerator.symbolResolution = true
                    BacktraceGenerator.start()
                } else {
                    console.warn("Unexpected install button state :O")
                }
            }
            icon.name: "install"
            tooltip: i18nc("@info:tooltip", "Use this button to install the packages for missing debug symbols.")
        },

        Kirigami.Action {
            id: reportAction
            enabled: (Kirigami.Settings.isMobile ? true : canReport) && BacktraceGenerator.state === BacktraceGenerator.Loaded
            visible: Kirigami.Settings.isMobile ? canReport : true
            icon.name: "story-editor-symbolic"
            text: i18nc("@action Report the bug on this domain", "Report on %1", Globals.bugzillaShortUrl)
            tooltip: canReportText !== "" ? canReportText : i18nc("@info:tooltip", "Starts the bug report assistant.")
            onTriggered: pageStack.push("qrc:/ui/WelcomePage.qml")
        },

        Kirigami.Action {
            id: reloadAction
            enabled: BacktraceGenerator.state !== BacktraceGenerator.Loading
            icon.name: "view-refresh"
            text: i18nc("@action:button", "Reload")
            tooltip: xi18nc("@info:tooltip",
`Use this button to reload the crash information (backtrace). This is useful when you have
installed the proper debug symbol packages and you want to obtain a better backtrace.`)
            onTriggered: {
                traceArea.text = ""
                BacktraceGenerator.start()
            }
        },

        Kirigami.Action {
            icon.name: "edit-copy"
            text: i18nc("@action:button", "Copy")
            tooltip: i18nc("@info:tooltip", "Use this button to copy the crash information (backtrace) to the clipboard.")
            enabled: BacktraceGenerator.state === BacktraceGenerator.Loaded
            onTriggered: DrKonqi.copyToClipboard(traceArea.text)
        },

        Kirigami.Action {
            icon.name: "document-save"
            text: i18nc("@action:button", "Save")
            tooltip: xi18nc("@info:tooltip",
`Use this button to save the crash information (backtrace) to a file. This is useful if you want to take a look at it or to report the bug later.`)
            enabled: BacktraceGenerator.state === BacktraceGenerator.Loaded
            onTriggered: DrKonqi.saveReport(traceArea.text)
        }
    ]

    header: QQC2.ToolBar {
        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                text: i18nc("@info", "This backtrace is very limited because there was not enough memory available to generate a full backtrace. You can close some applications and try again.")
                type: Kirigami.MessageType.Information
                visible: BacktraceGenerator.crampedMemory
                actions: [
                    Kirigami.Action {
                        text: i18nc("@action retry gathering crash data", "Retry")
                        onTriggered: BacktraceGenerator.start()
                    }
                ]
            }

            RatingItem {
                id: ratingItem
                failed: BacktraceGenerator.state === BacktraceGenerator.Failed || BacktraceGenerator.state === BacktraceGenerator.FailedToStart || BacktraceGenerator.state === BacktraceGenerator.MemoryPressure
                loading: BacktraceGenerator.state === BacktraceGenerator.Loading
            }

            DownloadSymbolsCheckBox {
                Layout.maximumWidth: Math.min(implicitWidth, page.width - page.leftPadding - page.rightPadding)
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    ColumnLayout {
        spacing: 0

        DebugPackageInstaller { // not in global scope because it messes up scrollbars
            id: debugPackageInstaller
            onPackagesInstalled: reloadAction.trigger()
            onError: appWindow.showPassiveNotification(i18nc("@title:window", "Error during the installation of debug symbols"), "long")
        }

        QQC2.TextArea {
            id: traceArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: text !== ""
            background: Rectangle {
                color: Kirigami.Theme.backgroundColor
            }

            // text: output.text
            font: Kirigami.Theme.fixedWidthFont
            wrapMode: TextEdit.Wrap
            textFormat: TextEdit.PlainText
            readOnly: true
            selectByMouse: Kirigami.Settings.isMobile ? false : true

            SyntaxHighlighter {
                textEdit:  BacktraceGenerator.debuggerIsGDB() ? traceArea : undefined
                definition: "GDB Backtrace"
            }

            function ensureVisible(r) {
                if (flickable.contentX >= r.x) {
                    flickable.contentX = r.x;
                } else if (flickable.contentX + flickable.width <= r.x + r.width) {
                    flickable.contentX = r.x + r.width - flickable.width;
                }
                if (flickable.contentY >= r.y) {
                    flickable.contentY = r.y;
                } else if (flickable.contentY + flickable.height <= r.y + r.height) {
                    flickable.contentY = r.y + r.height - flickable.height;
                }
            }

            Connections {
                id: generatorConnections
                target: BacktraceGenerator
                function onNewLine(line) { traceArea.text += line }
                function onStateChanged() {
                    console.log(BacktraceGenerator.state)
                    console.log(BacktraceGenerator.Loaded)

                    const state = BacktraceGenerator.state
                    page.state = state

                    installButton.visible = false

                    const parser = BacktraceGenerator.parser();
                    usefulness = parser.backtraceUsefulness()
                    // ratingItem.usefulness = usefulness
                    if (state == BacktraceGenerator.Loaded) {
                        traceArea.text = BacktraceGenerator.backtrace()
                        // Kinda hacky. Scroll all the way down, then scroll up until the handler is visible.
                        // This should bring the most relevant frames into the viewport.
                        traceArea.cursorPosition = traceArea.length - 1
                        traceArea.ensureVisible(traceArea.cursorRectangle)
                        traceArea.cursorPosition = traceArea.text.indexOf("[KCrash Handler]")
                        traceArea.ensureVisible(traceArea.cursorRectangle)
                        trace = traceArea.text // FIXME ensure this doesn't result in a binding

                        if (usefulness != BacktraceParser.ReallyUseful) {
                            if (debugPackageInstaller.canInstallDebugPackages || BacktraceGenerator.supportsSymbolResolution) {
                                detailsLabel.text = xi18nc("@info/rich",
`Click the <interface>Install Debug Symbols</interface> button in order to automatically install the missing debugging information packages. If this method
does not work, read <link url='%1'>How to create useful crash reports</link> to learn how to get a useful
backtrace, install the needed packages (<link url='%2'>list of files</link>), then click the <interface>Reload</interface> button.`,
                                                            Globals.techbaseHowtoDoc, '#missingDebugPackages')
                                installButton.visible = true
                                debugPackageInstaller.setMissingLibraries(parser.librariesWithMissingDebugSymbols())
                            } else {
                                detailsLabel.text = xi18nc("@info/rich",
`Read <link url='%1'>How to create useful crash reports</link> to learn how to get a useful backtrace, install the needed packages
(<link url='%2'>list of files</link>), then click the <interface>Reload</interface> button.`,
                                                            Globals.techbaseHowtoDoc, '#missingDebugPackages')
                            }
                        }
                    } else if (state == BacktraceGenerator.Failed) {
                        traceArea.text = BacktraceGenerator.rawTraceData()
                        detailsLabel.text = xi18nc("@info/rich", `Try to regenerate the backtrace by clicking the <interface>Reload</interface> button.`)
                    } else if (state == BacktraceGenerator.FailedToStart) {
                        traceArea.text = BacktraceGenerator.rawTraceData()
                        detailsLabel.text = xi18nc("@info/rich",
`<emphasis strong='true'>First install the debugger application (%1), then click the <interface>Reload</interface> button.</emphasis>`,
                                                   BacktraceGenerator.debuggerName())
                    } else if (state == BacktraceGenerator.MemoryPressure) {
                        traceArea.text = BacktraceGenerator.rawTraceData()
                        detailsLabel.text = xi18nc("@info/rich", `The backtrace generation was stopped due to memory pressure. You could try to regenerate the backtrace by clicking the <interface>Reload</interface> button.`)
                    }
                }
            }
        }
    }

    footer: QQC2.ToolBar {
        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Control { // Get standard padding so it doesn't stick to the edges, Label has none by default cause it isn't a Control
                Layout.fillWidth: true
                background: null

                Kirigami.PromptDialog {
                    id: filesDialog
                    title: i18nc("@title", "Not Sufficiently Useful")
                    function reload() {
                        const parser = BacktraceGenerator.parser()
                        let missingDbgForFiles = parser.librariesWithMissingDebugSymbols()
                        missingDbgForFiles.unshift(CrashedApplication.exectuableAbsoluteFilePath)
                        // TODO should maybe prepend DrKonqi::crashedApplication()->executable().absoluteFilePath()
                        // NB: cannot use xi18nc here because that'd close the html tag but we need to append an unordered list of paths
                        let message = "<html>" + i18n("The packages containing debug information for the following application and libraries are missing:") + "<br /><ul>";
                        for (const i in missingDbgForFiles) {
                            message += "<li>" + missingDbgForFiles[i] + "</li>";
                        }
                        message += "</ul></html>"
                        subtitle = message
                    }

                    showCloseButton: true
                }

                contentItem: QQC2.Label {
                    id: detailsLabel
                    wrapMode: Text.Wrap
                    onLinkActivated: link => {
                        if (link[0] == "#") { // in-page reference
                            filesDialog.reload()
                            filesDialog.open()
                        } else {
                            Qt.openUrlExternally(link)
                        }
                    }
                    textFormat: Text.RichText
                }
            }
            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.ActionToolBar {
                    id: footerBar
                    alignment: Qt.AlignRight
                    flat: false
                }
            }
        }
    }

    Component.onCompleted: {
        if (BacktraceGenerator.state === BacktraceGenerator.NotLoaded) {
            reloadAction.trigger()
        } else {
            // ensure our current state is the state of the backend. this is important in case the developer page
            // was used before the report workflow (i.e. the data was generated but from a different view)
            generatorConnections.onStateChanged()
        }
    }
}
