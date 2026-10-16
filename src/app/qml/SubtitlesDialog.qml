import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Subtitle files of the open video (I-57). Changes apply at once and are saved in the history.
Dialog {
    id: dialog

    // "main" or "secondary": the file being chosen.
    property string target: "main"

    signal exportRequested()

    function chooseFile(fileTarget) {
        target = fileTarget
        fileDialog.currentFolder = App.videoFolderUrl
        fileDialog.open()
    }

    component SubtitleFileRow: RowLayout {
        id: fileRow

        required property string caption
        required property string path

        signal chooseRequested()
        signal clearRequested()

        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.preferredWidth: 130
            text: fileRow.caption
        }

        Label {
            Layout.fillWidth: true
            text: fileRow.path !== "" ? App.fileName(fileRow.path) : qsTr("not selected")
            elide: Text.ElideMiddle
            font.italic: fileRow.path === ""
        }

        Button {
            text: qsTr("Choose…")
            focusPolicy: Qt.NoFocus

            onClicked: fileRow.chooseRequested()
        }

        Button {
            text: qsTr("Remove")
            focusPolicy: Qt.NoFocus
            enabled: fileRow.path !== ""

            onClicked: fileRow.clearRequested()
        }
    }

    width: Math.min(720, parent ? parent.width - 40 : 720)
    modal: true
    title: qsTr("Subtitles")

    onAboutToShow: App.refreshFoundSubtitles()
    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            focusPolicy: Qt.NoFocus
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        SubtitleFileRow {
            caption: qsTr("Main:")
            path: App.mainSubtitlesPath

            onChooseRequested: dialog.chooseFile("main")
            onClearRequested: App.setMainSubtitles("")
        }

        SubtitleFileRow {
            caption: qsTr("Secondary:")
            path: App.secondarySubtitlesPath

            onChooseRequested: dialog.chooseFile("secondary")
            onClearRequested: App.setSecondarySubtitles("")
        }

        Label {
            Layout.fillWidth: true
            visible: App.subtitlesAutomatic
            text: qsTr("Found automatically. Choosing or removing a file saves the choice for this video.")
            wrapMode: Text.Wrap
            color: palette.placeholderText
        }

        // Subtitle files found next to the video (I-06).
        Label {
            Layout.topMargin: 4
            visible: App.foundSubtitles.length > 0
            text: qsTr("Found next to the video:")
            font.bold: true
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 240)
            visible: App.foundSubtitles.length > 0
            clip: true
            spacing: 4
            model: App.foundSubtitles
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}

            delegate: RowLayout {
                id: foundRow

                required property var modelData

                width: ListView.view.width
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: foundRow.modelData.relativePath
                        elide: Text.ElideMiddle
                    }

                    Label {
                        Layout.fillWidth: true
                        text: foundRow.modelData.description
                        color: palette.placeholderText
                        elide: Text.ElideRight
                    }
                }

                Button {
                    text: qsTr("Main")
                    focusPolicy: Qt.NoFocus
                    enabled: foundRow.modelData.path !== App.mainSubtitlesPath

                    onClicked: App.setMainSubtitles(foundRow.modelData.path)
                }

                Button {
                    text: qsTr("Secondary")
                    focusPolicy: Qt.NoFocus
                    enabled: foundRow.modelData.path !== App.secondarySubtitlesPath

                    onClicked: App.setSecondarySubtitles(foundRow.modelData.path)
                }
            }
        }

        Button {
            text: qsTr("Export embedded…")
            focusPolicy: Qt.NoFocus
            visible: App.embeddedTextTrackCount > 0

            onClicked: {
                dialog.close()
                dialog.exportRequested()
            }
        }
    }

    FileDialog {
        id: fileDialog

        title: dialog.target === "main" ? qsTr("Main subtitles") : qsTr("Secondary subtitles")
        nameFilters: [qsTr("Subtitles (*.srt *.vtt *.ass *.ssa)"), qsTr("All files (*)")]

        onAccepted: {
            if (dialog.target === "main") {
                App.setMainSubtitles(selectedFile)
            } else {
                App.setSecondarySubtitles(selectedFile)
            }
        }
    }
}
