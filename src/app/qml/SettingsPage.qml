import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Settings page (I-50): changes are applied and saved at once.
Page {
    id: settingsPage

    property string mpvVersion: ""

    signal closeRequested()

    // The wheel outside the scrollable content does not reach the player below (no volume change).
    wheelEnabled: true

    component Section: GroupBox {
        Layout.fillWidth: true
        Layout.maximumWidth: 900
    }

    // Sample text in the colours of the subtitles over the video (SubtitleOverlay).
    component OverlaySample: Label {
        Layout.fillWidth: true
        Layout.maximumWidth: implicitWidth
        text: qsTr("Sample text")
        color: "#ffffff"
        elide: Text.ElideRight
        leftPadding: 12
        rightPadding: 12
        topPadding: 4
        bottomPadding: 4

        background: Rectangle {
            radius: 4
            color: "#d0000000"
        }
    }

    header: ToolBar {
        Label {
            anchors.centerIn: parent
            text: qsTr("Settings")
            font.pixelSize: 18
            font.bold: true
        }

        ToolButton {
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            text: "✕"
            font.pixelSize: 22
            focusPolicy: Qt.NoFocus

            onClicked: settingsPage.closeRequested()

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Close (Esc)")
        }
    }

    ScrollView {
        id: scrollView

        anchors.fill: parent
        contentWidth: availableWidth

        // The sections are centred in the window.
        ColumnLayout {
            width: Math.min(900, scrollView.availableWidth - 48)
            x: (scrollView.availableWidth - width) / 2
            spacing: 16

            Item {
                Layout.preferredHeight: 4
            }

            Section {
                title: qsTr("Languages")

                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 8

                    Label {
                        text: qsTr("Studied language")
                    }

                    LanguagePicker {
                        Layout.preferredWidth: 320
                        code: App.studiedLanguage

                        onPicked: code => App.studiedLanguage = code
                    }

                    Label {
                        text: qsTr("Native language")
                    }

                    LanguagePicker {
                        Layout.preferredWidth: 320
                        code: App.nativeLanguage

                        onPicked: code => App.nativeLanguage = code
                    }

                    Label {
                        text: qsTr("Audio language")
                    }

                    LanguagePicker {
                        Layout.preferredWidth: 320
                        code: App.preferredAudioLanguage
                        allowEmpty: true
                        emptyText: qsTr("Same as studied")

                        onPicked: code => App.preferredAudioLanguage = code
                    }
                }
            }

            Section {
                title: qsTr("Dictionary")

                ColumnLayout {
                    anchors.fill: parent

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("A click on a word opens the dictionary. In the URL template {text} is the word, {src} and {dst} are the studied and native languages.")
                        wrapMode: Text.Wrap
                        color: palette.placeholderText
                    }

                    UrlTemplateField {
                        Layout.fillWidth: true
                        source: App.dictionaryUrl
                        save: url => App.setDictionaryUrl(url)
                        reset: () => App.resetDictionaryUrl()
                    }
                }
            }

            Section {
                title: qsTr("Translator")

                ColumnLayout {
                    anchors.fill: parent

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("The “G” button of a cue opens the translator with the cue text.")
                        wrapMode: Text.Wrap
                        color: palette.placeholderText
                    }

                    UrlTemplateField {
                        Layout.fillWidth: true
                        source: App.translatorUrl
                        save: url => App.setTranslatorUrl(url)
                        reset: () => App.resetTranslatorUrl()
                    }
                }
            }

            Section {
                title: qsTr("Subtitles")

                // Font sizes of the subtitle list and of the subtitles over the video, with samples in their colours.
                GridLayout {
                    anchors.fill: parent
                    columns: 3
                    columnSpacing: 16
                    rowSpacing: 8

                    Label {
                        Layout.columnSpan: 3
                        text: qsTr("List on pause")
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Main subtitles font size")
                    }

                    NumberField {
                        from: App.minimumSubtitleFontSize
                        to: App.maximumSubtitleFontSize
                        value: App.mainSubtitleFontSize

                        onValueModified: App.mainSubtitleFontSize = value
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Sample text")
                        color: "#111111"
                        font.pixelSize: App.mainSubtitleFontSize
                        elide: Text.ElideRight
                    }

                    Label {
                        text: qsTr("Secondary subtitles font size")
                    }

                    NumberField {
                        from: App.minimumSubtitleFontSize
                        to: App.maximumSubtitleFontSize
                        value: App.secondarySubtitleFontSize

                        onValueModified: App.secondarySubtitleFontSize = value
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Sample text")
                        color: "#146c2e"
                        font.pixelSize: App.secondarySubtitleFontSize
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.columnSpan: 3
                        Layout.topMargin: 8
                        text: qsTr("On video (main at the top, secondary at the bottom)")
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Main subtitles font size")
                    }

                    NumberField {
                        from: App.minimumSubtitleFontSize
                        to: App.maximumSubtitleFontSize
                        value: App.mainOverlayFontSize

                        onValueModified: App.mainOverlayFontSize = value
                    }

                    OverlaySample {
                        font.pixelSize: App.mainOverlayFontSize
                    }

                    Label {
                        text: qsTr("Secondary subtitles font size")
                    }

                    NumberField {
                        from: App.minimumSubtitleFontSize
                        to: App.maximumSubtitleFontSize
                        value: App.secondaryOverlayFontSize

                        onValueModified: App.secondaryOverlayFontSize = value
                    }

                    OverlaySample {
                        font.pixelSize: App.secondaryOverlayFontSize
                    }
                }
            }

            Section {
                title: qsTr("History")

                RowLayout {
                    anchors.fill: parent
                    spacing: 16

                    Label {
                        text: qsTr("Files to keep")
                    }

                    NumberField {
                        from: 1
                        to: 1000
                        value: App.historySize

                        onValueModified: App.historySize = value
                    }

                    CheckBox {
                        Layout.leftMargin: 16
                        text: qsTr("Start watched files from the beginning")
                        checked: App.restartWatched
                        focusPolicy: Qt.NoFocus

                        onToggled: App.restartWatched = checked

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("A file played to at least 95% is watched.")
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Clear history…")
                        focusPolicy: Qt.NoFocus

                        onClicked: clearDialog.open()
                    }
                }
            }

            Section {
                title: qsTr("About")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    Label {
                        text: "AF Video Player " + App.appVersion
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Free software, licensed under GPL-3.0-or-later.")
                    }

                    Label {
                        text: "<a href=\"https://github.com/andrew2020wit/af-video-player-v2\">github.com/andrew2020wit/af-video-player-v2</a>"
                        textFormat: Text.StyledText

                        onLinkActivated: link => Qt.openUrlExternally(link)

                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }
                    }

                    Label {
                        Layout.topMargin: 8
                        text: "Qt " + App.qtVersion + " · " + settingsPage.mpvVersion + " · FFmpeg " + App.ffmpegVersion
                        color: palette.placeholderText
                    }
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    Dialog {
        id: clearDialog

        anchors.centerIn: parent
        modal: true
        title: qsTr("Clear history?")
        footer: DialogButtonBox {
            Button {
                text: qsTr("Clear")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }

            Button {
                text: qsTr("Cancel")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }

        onAccepted: App.clearHistory()

        Label {
            text: qsTr("Positions, subtitles and audio tracks of all files will be forgotten.")
        }
    }
}
