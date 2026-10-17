import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    id: window

    property bool settingsOpen: false
    // The video was playing when the settings were opened: it continues when they are closed.
    property bool resumeAfterSettings: false
    property int visibilityBeforeFullScreen: Window.Windowed
    // Geometry of the normal (not maximized) window, saved on close (I-75).
    property rect normalGeometry: Qt.rect(0, 0, 0, 0)

    // Shows the window as it was at the last close (I-75).
    function restoreGeometry() {
        const saved = App.windowGeometry()

        if (saved.valid) {
            window.x = saved.x
            window.y = saved.y
            window.width = saved.width
            window.height = saved.height
        }

        normalGeometry = Qt.rect(window.x, window.y, window.width, window.height)

        if (saved.valid && saved.fullScreen) {
            visibilityBeforeFullScreen = saved.maximized ? Window.Maximized : Window.Windowed
            window.showFullScreen()
        } else if (saved.valid && saved.maximized) {
            window.showMaximized()
        } else {
            window.show()
        }
    }

    function saveGeometry() {
        if (window.visibility === Window.Windowed) {
            normalGeometry = Qt.rect(window.x, window.y, window.width, window.height)
        }

        const fullScreen = window.visibility === Window.FullScreen
        const maximized = fullScreen ? visibilityBeforeFullScreen === Window.Maximized
                                     : window.visibility === Window.Maximized
        App.saveWindowGeometry(normalGeometry.x, normalGeometry.y, normalGeometry.width, normalGeometry.height,
                               maximized, fullScreen)
    }

    function toggleFullScreen() {
        if (window.visibility === Window.FullScreen) {
            exitFullScreen()
        } else {
            visibilityBeforeFullScreen = window.visibility
            window.showFullScreen()
        }
    }

    function exitFullScreen() {
        if (window.visibility !== Window.FullScreen) {
            return
        }

        if (visibilityBeforeFullScreen === Window.Maximized) {
            window.showMaximized()
        } else {
            window.showNormal()
        }
    }

    function openSettings() {
        resumeAfterSettings = player.mpv.hasFile && !player.mpv.paused

        if (player.mpv.hasFile) {
            player.mpv.paused = true
        }

        settingsOpen = true
    }

    function closeSettings() {
        settingsOpen = false

        if (resumeAfterSettings && player.mpv.hasFile) {
            player.mpv.paused = false
        }

        resumeAfterSettings = false
        root.forceActiveFocus()
    }

    function restoreFocus() {
        const item = window.activeFocusItem

        if (window.active && (!item || item === window.contentItem || item === window.contentItem.parent)) {
            root.forceActiveFocus()
        }
    }

    function openVideoDialog() {
        videoDialog.currentFolder = App.openFolderUrl
        videoDialog.selectedFile = App.openFileUrl
        videoDialog.open()
    }

    // Batch export (I-87): one at a time; the button brings its window to the front while it runs.
    function startBatchExport() {
        if (App.batchExport.running) {
            batchWindow.present()
        } else {
            batchFolderDialog.currentFolder = App.openFolderUrl
            batchFolderDialog.open()
        }
    }

    function goToStartPage() {
        settingsOpen = false
        resumeAfterSettings = false
        player.closeVideo()
        root.forceActiveFocus()
    }

    // Shortcuts work on every page; the rest of the keys belong to the player (SPEC 2.4).
    function handleKey(event) {
        const key = App.physicalKey(event.key)

        if (event.modifiers & Qt.ControlModifier) {
            if (key === Qt.Key_O) {
                openVideoDialog()
                event.accepted = true
            } else if (key === Qt.Key_H) {
                if (App.videoPath !== "") {
                    goToStartPage()
                }

                event.accepted = true
            } else if (key === Qt.Key_Comma) {
                if (!settingsOpen) {
                    openSettings()
                }

                event.accepted = true
            } else if (key === Qt.Key_Space) {
                // Play/pause without the subtitle list (F-11).
                if (!settingsOpen && !event.isAutoRepeat && player.mpv.hasFile) {
                    player.togglePause()
                }

                event.accepted = true
            }

            return
        }

        if (settingsOpen) {
            if (key === Qt.Key_Escape) {
                closeSettings()
                event.accepted = true
            }

            return
        }

        if (App.videoPath !== "") {
            player.handleKey(event, key)
        } else if (key === Qt.Key_F) {
            // Full screen on the start page as well (I-30).
            toggleFullScreen()
            event.accepted = true
        } else if (key === Qt.Key_Escape) {
            exitFullScreen()
            event.accepted = true
        }
    }

    width: 1280
    height: 760
    minimumWidth: 640
    minimumHeight: 400
    // One light palette on every system, independent of the system theme and accent colour, for all controls and
    // popups (SPEC 3.5): text has a contrast of at least 4.5:1 to its background in every state.
    palette.window: "#f0f0f0"
    palette.windowText: "#111111"
    palette.base: "#ffffff"
    palette.alternateBase: "#f5f5f5"
    palette.text: "#111111"
    palette.placeholderText: "#4b5563"
    palette.button: "#e8e8e8"
    palette.buttonText: "#111111"
    palette.brightText: "#ffffff"
    palette.light: "#ffffff"
    palette.midlight: "#e3e3e3"
    palette.mid: "#a0a0a0"
    palette.dark: "#8a8a8a"
    palette.shadow: "#6b6b6b"
    palette.highlight: "#1d4ed8"
    palette.highlightedText: "#ffffff"
    palette.accent: "#1d4ed8"
    palette.link: "#1d4ed8"
    palette.linkVisited: "#6d28d9"
    palette.toolTipBase: "#ffffdc"
    palette.toolTipText: "#111111"
    palette.disabled.windowText: "#626262"
    palette.disabled.text: "#626262"
    palette.disabled.buttonText: "#626262"
    palette.disabled.placeholderText: "#626262"
    palette.disabled.highlight: "#6b7280"
    title: (App.videoPath !== "" ? App.videoName + " — " : "") + "AF Video Player " + App.appVersion

    Component.onCompleted: restoreGeometry()
    onClosing: {
        player.savePlayback()
        saveGeometry()
        // The application quits with the main window.
        App.batchExport.cancel()
        batchWindow.close()
    }
    onXChanged: normalGeometryTimer.restart()
    onYChanged: normalGeometryTimer.restart()
    onWidthChanged: normalGeometryTimer.restart()
    onHeightChanged: normalGeometryTimer.restart()
    // Keys are handled by the root scope: focus left on the window itself (a closed popup, a hidden item) goes back.
    onActiveFocusItemChanged: Qt.callLater(window.restoreFocus)

    FocusScope {
        id: root

        anchors.fill: parent
        focus: true

        Keys.onPressed: event => window.handleKey(event)

        PlayerPage {
            id: player

            anchors.fill: parent
            visible: App.videoPath !== ""
            covered: window.settingsOpen

            onFullScreenToggleRequested: window.toggleFullScreen()
            onExitFullScreenRequested: window.exitFullScreen()
            onOpenFileRequested: window.openVideoDialog()
            onStartPageRequested: window.goToStartPage()
            onSettingsRequested: window.openSettings()
            onFocusRequested: root.forceActiveFocus()
        }

        StartPage {
            anchors.fill: parent
            visible: App.videoPath === ""

            onOpenFileRequested: window.openVideoDialog()
            onSettingsRequested: window.openSettings()
            onFullScreenRequested: window.toggleFullScreen()
            onBatchExportRequested: window.startBatchExport()
            onHistoryItemClicked: path => {
                player.openVideo(path)
                root.forceActiveFocus()
            }
        }

        SettingsPage {
            anchors.fill: parent
            visible: window.settingsOpen
            mpvVersion: player.mpv.mpvVersion

            onCloseRequested: window.closeSettings()
        }

        DropArea {
            anchors.fill: parent

            onDropped: drop => {
                if (drop.hasUrls) {
                    player.openDropped(drop.urls[0])
                    root.forceActiveFocus()
                }
            }
        }
    }

    // The normal geometry is taken a moment after a change, when a switch to or from the maximized or full screen
    // state has settled.
    Timer {
        id: normalGeometryTimer

        interval: 500

        onTriggered: {
            if (window.visibility === Window.Windowed) {
                window.normalGeometry = Qt.rect(window.x, window.y, window.width, window.height)
            }
        }
    }

    FileDialog {
        id: videoDialog

        title: qsTr("Open video")
        nameFilters: [qsTr("Videos (*.mkv *.mp4 *.m4v *.avi *.mov *.webm *.wmv *.ts *.m2ts *.mts *.mpg *.mpeg *.flv *.ogv *.3gp)"), qsTr("All files (*)")]

        onAccepted: {
            player.openVideo(selectedFile)
            root.forceActiveFocus()
        }
    }

    FolderDialog {
        id: batchFolderDialog

        title: qsTr("Folder with videos")

        onAccepted: {
            App.batchExport.start(App.toLocalPath(selectedFolder))
            batchWindow.present()
        }
    }

    BatchExportWindow {
        id: batchWindow

        mainWindow: window
        palette: window.palette
    }

    Dialog {
        id: errorDialog

        property alias text: errorLabel.text

        anchors.centerIn: parent
        width: Math.min(560, window.width - 40)
        modal: true
        title: qsTr("Error")
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

        // Errors that come together (both subtitle files fail to load) are shown together.
        function onErrorOccurred(message) {
            errorDialog.text = errorDialog.visible ? errorDialog.text + "\n\n" + message : message
            errorDialog.open()
        }

        // A file opened while the player is running (I-74).
        function onOpenRequested(file) {
            window.settingsOpen = false
            window.resumeAfterSettings = false
            player.openVideo(file)
            root.forceActiveFocus()
        }
    }
}
