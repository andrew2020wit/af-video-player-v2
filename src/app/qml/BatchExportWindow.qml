import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Progress of the batch export (I-87): a window of its own, so the player can be used meanwhile. It closes itself
// when the export ends; closing it cancels the export.
ApplicationWindow {
    id: batchWindow

    // The window the progress window is first centred over.
    property Window mainWindow

    // Shows the window, or brings it to the front when it is already open.
    function present() {
        if (!visible && mainWindow) {
            x = mainWindow.x + (mainWindow.width - width) / 2
            y = mainWindow.y + (mainWindow.height - height) / 2
        }

        if (visibility === Window.Minimized) {
            showNormal()
        } else {
            show()
        }

        raise()
        requestActivate()
    }

    // Not owned by the main window: it is neither kept above it nor minimized with it.
    transientParent: null
    width: 480
    height: content.implicitHeight + 32
    minimumWidth: 360
    title: qsTr("Batch export of subtitles")
    color: palette.window

    onClosing: App.batchExport.cancel()

    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: App.batchExport.folder
            elide: Text.ElideMiddle
        }

        Label {
            Layout.fillWidth: true
            text: App.batchExport.videoNumber > 0
                  ? qsTr("Video %1 of %2: %3").arg(App.batchExport.videoNumber).arg(App.batchExport.videoCount)
                                              .arg(App.batchExport.videoName)
                  : qsTr("Looking for videos…")
            elide: Text.ElideMiddle
        }

        ProgressBar {
            Layout.fillWidth: true
            value: App.batchExport.progress
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Subtitle files saved: %1").arg(App.batchExport.exportedCount)
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Cancel")
            focusPolicy: Qt.NoFocus

            onClicked: App.batchExport.cancel()
        }
    }

    Connections {
        target: App.batchExport

        function onFinished() {
            batchWindow.close()
        }
    }
}
