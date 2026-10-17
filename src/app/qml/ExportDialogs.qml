pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Export of embedded subtitles (I-08): choose tracks -> choose a folder -> resolve name conflicts ->
// extract in the background with progress -> offer to use the exported files.
Item {
    id: exportDialogs

    property var tracks: []
    property var selectedIndexes: []
    property string folder: ""
    property var exportedFiles: []

    function start() {
        tracks = App.embeddedTracks()
        selectedIndexes = []
        trackDialog.open()
    }

    function toggleTrack(streamIndex, checked) {
        const indexes = selectedIndexes.filter(index => index !== streamIndex)

        if (checked) {
            indexes.push(streamIndex)
        }

        selectedIndexes = indexes
    }

    function folderChosen(chosenFolder) {
        folder = chosenFolder
        const conflicts = App.exportConflicts(selectedIndexes, folder)

        if (conflicts.length > 0) {
            conflictDialog.names = conflicts
            conflictDialog.open()
        } else {
            App.startExport(selectedIndexes, folder, false)
        }
    }

    Dialog {
        id: trackDialog

        anchors.centerIn: parent
        width: Math.min(640, exportDialogs.width - 40)
        modal: true
        title: qsTr("Export embedded subtitles")

        footer: DialogButtonBox {
            Button {
                text: qsTr("Save…")
                enabled: exportDialogs.selectedIndexes.length > 0 && !App.exportRunning
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }

            Button {
                text: qsTr("Cancel")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }

        onAccepted: {
            folderDialog.currentFolder = App.videoFolderUrl
            folderDialog.open()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: qsTr("Select tracks: each one is saved to the chosen folder as a separate file.")
                wrapMode: Text.Wrap
            }

            Repeater {
                model: exportDialogs.tracks

                delegate: CheckBox {
                    required property var modelData

                    Layout.fillWidth: true
                    text: modelData.label
                    focusPolicy: Qt.NoFocus
                    checked: exportDialogs.selectedIndexes.includes(modelData.streamIndex)

                    onToggled: exportDialogs.toggleTrack(modelData.streamIndex, checked)
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog

        title: qsTr("Folder for subtitles")

        onAccepted: exportDialogs.folderChosen(selectedFolder)
    }

    Dialog {
        id: conflictDialog

        property var names: []

        anchors.centerIn: parent
        width: Math.min(560, exportDialogs.width - 40)
        modal: true
        title: qsTr("Files already exist")

        footer: DialogButtonBox {
            Button {
                text: qsTr("Replace")
                focusPolicy: Qt.NoFocus

                onClicked: {
                    conflictDialog.close()
                    App.startExport(exportDialogs.selectedIndexes, exportDialogs.folder, false)
                }
            }

            Button {
                text: qsTr("Keep both")
                focusPolicy: Qt.NoFocus

                onClicked: {
                    conflictDialog.close()
                    App.startExport(exportDialogs.selectedIndexes, exportDialogs.folder, true)
                }
            }

            Button {
                text: qsTr("Cancel")
                focusPolicy: Qt.NoFocus

                onClicked: {
                    conflictDialog.close()
                    trackDialog.open()
                }
            }
        }

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: qsTr("The folder already contains:") + "\n" + conflictDialog.names.join("\n")
        }
    }

    // Export runs in the background; the video keeps playing (SPEC 3.2).
    Popup {
        id: progressPopup

        x: exportDialogs.width - width - 16
        y: 16
        width: 320
        modal: false
        closePolicy: Popup.NoAutoClose
        visible: App.exportRunning

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                text: qsTr("Exporting subtitles…")
            }

            ProgressBar {
                Layout.fillWidth: true
                value: App.exportProgress
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Cancel")
                focusPolicy: Qt.NoFocus

                onClicked: App.cancelExport()
            }
        }
    }

    // Step 6 of the export scenario: use the exported files, pre-selected by language (I-55).
    Dialog {
        id: useDialog

        function fileOptions() {
            return [qsTr("— keep current —")].concat(exportDialogs.exportedFiles.map(file => file.name))
        }

        anchors.centerIn: parent
        width: Math.min(560, exportDialogs.width - 40)
        modal: true
        title: qsTr("Use the exported subtitles?")

        footer: DialogButtonBox {
            Button {
                text: qsTr("Use")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }

            Button {
                text: qsTr("Don't use")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }

        onAccepted: {
            if (mainBox.currentIndex > 0) {
                App.setMainSubtitles(exportDialogs.exportedFiles[mainBox.currentIndex - 1].path)
            }

            if (secondaryBox.currentIndex > 0) {
                App.setSecondarySubtitles(exportDialogs.exportedFiles[secondaryBox.currentIndex - 1].path)
            }
        }

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 12

            Label {
                text: qsTr("Main:")
            }

            ComboBox {
                id: mainBox

                Layout.fillWidth: true
                model: useDialog.fileOptions()
                focusPolicy: Qt.NoFocus
            }

            Label {
                text: qsTr("Secondary:")
            }

            ComboBox {
                id: secondaryBox

                Layout.fillWidth: true
                model: useDialog.fileOptions()
                focusPolicy: Qt.NoFocus
            }
        }
    }

    Dialog {
        id: errorDialog

        property alias text: errorLabel.text

        anchors.centerIn: parent
        width: Math.min(560, exportDialogs.width - 40)
        modal: true
        title: qsTr("Export failed")
        footer: DialogButtonBox {
            Button {
                text: qsTr("OK")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }

        Label {
            id: errorLabel

            width: parent.width
            wrapMode: Text.Wrap
        }
    }

    Connections {
        target: App

        function onExportFinished(ok, cancelled, error, files) {
            // No files: another video was opened during the export.
            if (ok && files.length > 0) {
                exportDialogs.exportedFiles = files
                mainBox.currentIndex = App.findFileByLanguage(files, App.studiedLanguage) + 1
                secondaryBox.currentIndex = App.findFileByLanguage(files, App.nativeLanguage) + 1
                useDialog.open()
            } else if (!ok && !cancelled) {
                errorDialog.text = error
                errorDialog.open()
            }
        }
    }
}
