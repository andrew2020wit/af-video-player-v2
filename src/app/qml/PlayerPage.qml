pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: page

    property alias mpv: mpv
    // The subtitle list covers the video (F-42). It is opened by Space and pauses the video; a pause by itself
    // (a click, Ctrl+Space) does not open it.
    property bool listOpen: false
    // Another page (the settings) covers the player: the mouse does not reach the video.
    property bool covered: false
    property bool controlsVisible: true
    // Time of the last chapter key: a chapter change within a second after it is shown in the OSD.
    property real chapterKeyTime: 0

    signal fullScreenToggleRequested()
    signal exitFullScreenRequested()
    signal openFileRequested()
    signal startPageRequested()
    signal settingsRequested()
    signal focusRequested()

    function savePlayback() {
        if (mpv.hasFile && App.videoPath !== "") {
            App.savePlayback(mpv.position, mpv.duration)
        }
    }

    function openVideo(location) {
        savePlayback()
        App.openVideo(location)
    }

    function openDropped(location) {
        savePlayback()
        App.openDropped(location)
    }

    function closeVideo() {
        savePlayback()
        listOpen = false
        mpv.stop()
        App.closeVideo()
    }

    // Opens the subtitle list; the video pauses (F-42).
    function openList() {
        if (mpv.hasFile) {
            listOpen = true
            mpv.paused = true
        }
    }

    // Closes the subtitle list and continues playback.
    function closeList() {
        listOpen = false
        mpv.paused = false
    }

    // Play/pause without the subtitle list (F-11, Ctrl+Space); an open list is closed.
    function togglePause() {
        if (listOpen) {
            closeList()
        } else {
            mpv.togglePause()
        }
    }

    function showControls() {
        controlsVisible = true
        hideControlsTimer.restart()
    }

    function changeVolume(delta) {
        App.volume = Math.max(0, Math.min(100, App.volume + delta))
        osd.show(qsTr("Volume: %1%").arg(App.volume))
    }

    function toggleMute() {
        App.muted = !App.muted
        osd.show(App.muted ? qsTr("Muted") : qsTr("Unmuted"))
    }

    // Subtitles over the video (I-22).
    function toggleSubtitleOverlay() {
        App.showSubtitleOverlay = !App.showSubtitleOverlay
        osd.show(App.showSubtitleOverlay ? qsTr("Subtitles on video: on") : qsTr("Subtitles on video: off"))
    }

    function selectAudioTrack(id) {
        mpv.setAudioTrack(id)
        App.setUserAudioTrack(id)

        for (const track of mpv.audioTracks) {
            if (track.id === id) {
                osd.show(qsTr("Audio: %1").arg(App.audioTrackLabel(track)))
            }
        }
    }

    // Previous (-1) or next (1) chapter (I-72); the OSD shows it when mpv reports the new chapter.
    function changeChapter(delta) {
        if (mpv.chapters.length === 0) {
            osd.show(qsTr("No chapters"))

            return
        }

        chapterKeyTime = Date.now()
        mpv.changeChapter(delta)
    }

    // Next video file in the folder (I-16).
    function openNextVideo() {
        const next = App.nextVideo()

        if (next === "") {
            osd.show(qsTr("This is the last file"))

            return
        }

        openVideo(next)
    }

    // Next audio track in a circle (I-02).
    function nextAudioTrack() {
        const tracks = mpv.audioTracks

        if (tracks.length < 2) {
            osd.show(qsTr("No other audio tracks"))

            return
        }

        const index = tracks.findIndex(track => track.id === mpv.audioTrackId)
        selectAudioTrack(tracks[(index + 1) % tracks.length].id)
    }

    // key: the physical key (App.physicalKey) of the event.
    function handleKey(event, key) {
        if (!mpv.hasFile) {
            return
        }

        event.accepted = true

        if (key === Qt.Key_F) {
            fullScreenToggleRequested()
        } else if (key === Qt.Key_M) {
            toggleMute()
        } else if (key === Qt.Key_N) {
            if (!event.isAutoRepeat) {
                openNextVideo()
            }
        } else if (listOpen) {
            handleListKey(event, key)
        } else if (key === Qt.Key_Space) {
            // The subtitle list (F-42).
            if (!event.isAutoRepeat) {
                openList()
            }
        } else if (key === Qt.Key_Escape) {
            exitFullScreenRequested()
        } else if (key === Qt.Key_Left) {
            mpv.seekRelative(-5)
            showControls()
        } else if (key === Qt.Key_Right) {
            mpv.seekRelative(5)
            showControls()
        } else if (key === Qt.Key_Up) {
            changeVolume(5)
        } else if (key === Qt.Key_Down) {
            changeVolume(-5)
        } else if (key === Qt.Key_A) {
            nextAudioTrack()
        } else if (key === Qt.Key_S) {
            if (!event.isAutoRepeat) {
                toggleSubtitleOverlay()
            }
        } else if (key === Qt.Key_PageUp) {
            changeChapter(-1)
        } else if (key === Qt.Key_PageDown) {
            changeChapter(1)
        } else {
            event.accepted = false
        }
    }

    function handleListKey(event, key) {
        if (key === Qt.Key_Space || key === Qt.Key_Escape) {
            if (!event.isAutoRepeat) {
                closeList()
            }
        } else if (key === Qt.Key_Up) {
            subtitleList.scrollBy(-60)
        } else if (key === Qt.Key_Down) {
            subtitleList.scrollBy(60)
        } else if (key === Qt.Key_PageUp) {
            subtitleList.scrollBy(-subtitleList.height * 0.9)
        } else if (key === Qt.Key_PageDown) {
            subtitleList.scrollBy(subtitleList.height * 0.9)
        } else if (key === Qt.Key_Home) {
            subtitleList.scrollToBeginning()
        } else if (key === Qt.Key_End) {
            subtitleList.scrollToEnd()
        } else if (key === Qt.Key_C) {
            subtitleList.scrollToCurrent()
        } else {
            event.accepted = false
        }
    }

    MpvPlayer {
        id: mpv

        anchors.fill: parent
        volume: App.volume
        muted: App.muted

        onFileLoaded: {
            const id = App.chooseAudioTrack(audioTracks)

            if (id > 0) {
                setAudioTrack(id)
            }
        }
        onChapterIndexChanged: {
            if (Date.now() - page.chapterKeyTime < 1000 && chapterIndex >= 0 && chapterIndex < chapters.length) {
                osd.show(App.chapterLabel(chapters[chapterIndex].title, chapterIndex))
            }
        }
        onLoadFailed: message => osd.show(qsTr("Cannot open the file: %1").arg(message))
        // At the end the video stops on the last frame and the list opens with the "Next" button (I-16).
        onEofReachedChanged: {
            subtitleList.nextVideoPath = eofReached ? App.nextVideo() : ""

            if (eofReached) {
                page.openList()
            }
        }
        onPausedChanged: {
            if (paused) {
                page.savePlayback()
            } else {
                // The list exists only on pause.
                page.listOpen = false
            }

            page.showControls()
        }
    }

    Connections {
        target: App

        function onVideoOpened(path, startSeconds) {
            page.listOpen = false
            subtitleList.reset()
            mpv.loadFile(path, startSeconds)
            page.focusRequested()
        }
    }

    // The display stays on and the system awake during playback (I-69).
    SleepInhibitor {
        active: mpv.hasFile && !mpv.paused
    }

    // Position is saved every 10 s during playback (I-14).
    Timer {
        interval: 10000
        repeat: true
        running: mpv.hasFile && !mpv.paused

        onTriggered: page.savePlayback()
    }

    // Panel and cursor hide after 2 s without mouse movement (F-14), but not on pause, while the mouse is over
    // the panel or the video menu is open.
    Timer {
        id: hideControlsTimer

        interval: 2000

        onTriggered: {
            if (!controlBar.keepVisible && !videoMenu.opened && !mpv.paused) {
                page.controlsVisible = false
            }
        }
    }

    // A single click is delayed by the double-click interval so that a double click (full screen) does not pause.
    Timer {
        id: clickTimer

        // qmllint disable missing-property
        // Qt.styleHints is typed as QObject for qmllint.
        interval: Qt.styleHints.mouseDoubleClickInterval
        // qmllint enable missing-property

        onTriggered: page.togglePause()
    }

    MouseArea {
        anchors.fill: parent
        enabled: !page.covered
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: page.controlsVisible || page.listOpen ? Qt.ArrowCursor : Qt.BlankCursor

        onPositionChanged: page.showControls()
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton) {
                if (mpv.hasFile) {
                    videoMenu.popup()
                }
            } else {
                clickTimer.restart()
            }
        }
        onDoubleClicked: mouse => {
            if (mouse.button === Qt.LeftButton) {
                clickTimer.stop()
                page.fullScreenToggleRequested()
            }
        }
        // The wheel scrolls the subtitle list when it is open and never changes the volume under it.
        onWheel: wheel => {
            if (!page.listOpen) {
                page.changeVolume(wheel.angleDelta.y > 0 ? 5 : -5)
            }
        }
    }

    SubtitleOverlay {
        anchors.fill: parent
        mpv: mpv
        visible: App.showSubtitleOverlay && mpv.hasFile && !page.listOpen
        bottomInset: controlBar.visible ? controlBar.height : 0
    }

    SubtitleList {
        id: subtitleList

        anchors.fill: parent
        mpv: mpv
        visible: page.listOpen

        onSubtitlesDialogRequested: subtitlesDialog.open()
        onExportRequested: exportDialogs.start()
        onNextVideoRequested: page.openNextVideo()
        onSettingsRequested: page.settingsRequested()
        onCloseRequested: page.closeList()
    }

    ControlBar {
        id: controlBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        mpv: mpv
        // Hidden under the subtitle list: the list takes the whole page.
        opacity: !page.listOpen && (page.controlsVisible || !mpv.hasFile) ? 1 : 0
        visible: opacity > 0

        onPlayPauseRequested: page.togglePause()
        onSubtitlesRequested: subtitlesDialog.open()
        onOpenFileRequested: page.openFileRequested()
        onStartPageRequested: page.startPageRequested()
        onSettingsRequested: page.settingsRequested()
        onFullScreenRequested: page.fullScreenToggleRequested()
        onInteracted: page.showControls()

        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }
    }

    // Right click on the video: subtitles over the video (I-22) and audio tracks (I-02).
    // A checkable item flips its own "checked" on a click, so the binding is restored after it.
    Menu {
        id: videoMenu

        modal: true
        dim: false

        onClosed: {
            page.showControls()
            page.focusRequested()
        }

        MenuItem {
            id: overlayItem

            text: qsTr("Subtitles on video (S)")
            checkable: true
            checked: App.showSubtitleOverlay

            onTriggered: {
                page.toggleSubtitleOverlay()
                overlayItem.checked = Qt.binding(() => App.showSubtitleOverlay)
            }
        }

        Menu {
            id: audioMenu

            title: qsTr("Audio track (A)")

            Instantiator {
                model: page.mpv.audioTracks

                delegate: MenuItem {
                    id: audioItem

                    required property var modelData

                    text: App.audioTrackLabel(audioItem.modelData)
                    checkable: true
                    checked: audioItem.modelData.id === page.mpv.audioTrackId

                    onTriggered: {
                        page.selectAudioTrack(audioItem.modelData.id)
                        audioItem.checked = Qt.binding(() => audioItem.modelData.id === page.mpv.audioTrackId)
                    }
                }

                onObjectAdded: (index, object) => audioMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => audioMenu.removeItem(object)
            }
        }
    }

    Osd {
        id: osd

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
    }

    SubtitlesDialog {
        id: subtitlesDialog

        anchors.centerIn: parent

        onExportRequested: exportDialogs.start()
    }

    ExportDialogs {
        id: exportDialogs

        anchors.fill: parent
    }
}
