pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Bottom control panel (F-12, I-56): play/pause, progress, time, subtitles, open file, start page, settings,
// full screen.
Rectangle {
    id: bar

    required property var mpv
    // The panel does not hide while the mouse is over it or the progress bar is dragged.
    readonly property bool keepVisible: hoverHandler.hovered || progressArea.pressed

    signal playPauseRequested()
    signal subtitlesRequested()
    signal openFileRequested()
    signal startPageRequested()
    signal settingsRequested()
    signal fullScreenRequested()
    signal interacted()

    // White on the dark panel in every state: the style's own hover background is light and is replaced.
    component BarButton: ToolButton {
        id: barButton

        property string toolTip

        Layout.preferredWidth: 40
        Layout.preferredHeight: 36
        focusPolicy: Qt.NoFocus
        font.pixelSize: 17
        // Monochrome symbols in the text colour, not colour emoji (on other systems the family falls back).
        font.family: "Segoe UI Symbol"
        palette.buttonText: "#ffffff"

        background: Rectangle {
            radius: 4
            color: barButton.down ? "#55ffffff" : barButton.hovered ? "#33ffffff" : "transparent"
        }

        ToolTip.visible: hovered && toolTip !== ""
        ToolTip.text: toolTip
        ToolTip.delay: 500
    }

    height: 52
    // Opaque enough for white text and the progress bar over any frame.
    color: "#e6101010"

    HoverHandler {
        id: hoverHandler

        onHoveredChanged: bar.interacted()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 6

        BarButton {
            text: bar.mpv.paused ? "▶" : "⏸"
            toolTip: bar.mpv.paused ? qsTr("Play (Ctrl+Space)") : qsTr("Pause (Ctrl+Space)")

            onClicked: bar.playPauseRequested()
        }

        // Click seeks to the position (F-13). Dragging seeks to keyframes on the way and exactly on release;
        // the time under the mouse is shown above the bar (I-34).
        Item {
            id: progress

            // Time the bar is dragged to, -1 when it is not dragged.
            property real dragSeconds: -1
            // A keyframe seek to dragSeconds waits for the throttle timer.
            property bool seekPending: false
            readonly property real shownSeconds: dragSeconds >= 0 ? dragSeconds : bar.mpv.position

            function timeAt(x) {
                return Math.max(0, Math.min(1, x / width)) * bar.mpv.duration
            }

            // Title of the chapter a time belongs to (I-72), empty without chapters.
            function chapterTitleAt(seconds) {
                const chapters = bar.mpv.chapters
                let index = -1

                for (let i = 0; i < chapters.length && chapters[i].time <= seconds; ++i) {
                    index = i
                }

                return index >= 0 ? App.chapterLabel(chapters[index].title, index) : ""
            }

            function finishDrag() {
                if (dragSeconds >= 0) {
                    bar.mpv.seek(dragSeconds)
                }

                dragSeconds = -1
                seekPending = false
            }

            Layout.fillWidth: true
            Layout.preferredHeight: 28

            Rectangle {
                id: track

                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 6
                radius: 3
                // At least 3:1 to the panel and to the played part (SPEC 3.5).
                color: "#747474"

                Rectangle {
                    width: bar.mpv.duration > 0 ? parent.width * Math.min(1, progress.shownSeconds / bar.mpv.duration) : 0
                    height: parent.height
                    radius: 3
                    color: "#bfdbfe"
                }

                // Chapter starts (I-72); the first chapter usually starts at 0 and needs no mark. White with a black
                // edge, so the mark stands out on the panel, the track and the played part.
                Repeater {
                    model: bar.mpv.duration > 0 ? bar.mpv.chapters : []

                    delegate: Rectangle {
                        required property var modelData

                        x: track.width * modelData.time / bar.mpv.duration - width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        width: 4
                        height: 14
                        color: "#ffffff"
                        border.width: 1
                        border.color: "#000000"
                        visible: modelData.time > 0 && modelData.time < bar.mpv.duration
                    }
                }
            }

            MouseArea {
                id: progressArea

                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
                cursorShape: Qt.PointingHandCursor

                onPressed: mouse => {
                    if (bar.mpv.duration > 0) {
                        progress.dragSeconds = progress.timeAt(mouse.x)
                    }
                }
                onPositionChanged: mouse => {
                    if (pressed && progress.dragSeconds >= 0) {
                        progress.dragSeconds = progress.timeAt(mouse.x)
                        progress.seekPending = true
                    }
                }
                onReleased: progress.finishDrag()
                onCanceled: progress.finishDrag()
            }

            // At most one keyframe seek per interval while dragging.
            Timer {
                interval: 100
                repeat: true
                running: progressArea.pressed

                onTriggered: {
                    if (progress.seekPending) {
                        progress.seekPending = false
                        bar.mpv.seekKeyframe(progress.dragSeconds)
                    }
                }
            }

            // Frame (I-73) and time under the mouse.
            Rectangle {
                id: hoverTime

                // While dragging the mouse may leave the bar.
                readonly property real atX: Math.max(0, Math.min(progress.width, progressArea.mouseX))
                readonly property bool shown: (progressArea.containsMouse || progressArea.pressed) && bar.mpv.duration > 0
                // Frames are asked for in steps of 1/400 of the video, at least 1 s, which bounds the decoding work.
                readonly property real frameStep: Math.max(1, bar.mpv.duration / 400)

                x: Math.max(0, Math.min(progress.width - width, atX - width / 2))
                y: -height - 6
                width: Math.max(hoverLabel.implicitWidth, frameImage.visible ? frameImage.width : 0) + 8
                height: hoverColumn.implicitHeight + 8
                radius: 4
                color: "#e6101010"
                visible: shown

                Column {
                    id: hoverColumn

                    anchors.centerIn: parent
                    spacing: 4

                    Image {
                        id: frameImage

                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 192
                        height: implicitWidth > 0 ? width * implicitHeight / implicitWidth : 0
                        visible: status === Image.Ready && implicitWidth > 0
                        asynchronous: true
                        cache: false
                        retainWhileLoading: true
                        fillMode: Image.PreserveAspectFit
                        source: hoverTime.shown
                                ? "image://frame/" + (Math.floor(progress.timeAt(hoverTime.atX) / hoverTime.frameStep)
                                                      * hoverTime.frameStep).toFixed(3)
                                : ""
                    }

                    Label {
                        id: hoverLabel

                        anchors.horizontalCenter: parent.horizontalCenter
                        leftPadding: 4
                        rightPadding: 4
                        text: {
                            const seconds = progress.timeAt(hoverTime.atX)
                            const chapter = progress.chapterTitleAt(seconds)

                            return App.formatTime(seconds) + (chapter !== "" ? " · " + chapter : "")
                        }
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.family: "monospace"
                    }
                }
            }
        }

        Label {
            text: App.formatTime(progress.shownSeconds) + " / " + App.formatTime(bar.mpv.duration)
            color: "#ffffff"
            font.pixelSize: 14
            font.family: "monospace"
        }

        BarButton {
            text: "CC"
            font.pixelSize: 14
            font.bold: true
            toolTip: qsTr("Subtitles")

            onClicked: bar.subtitlesRequested()
        }

        BarButton {
            text: "⏏"
            toolTip: qsTr("Open file (Ctrl+O)")

            onClicked: bar.openFileRequested()
        }

        BarButton {
            text: "⌂"
            toolTip: qsTr("Start page (Ctrl+H)")

            onClicked: bar.startPageRequested()
        }

        BarButton {
            text: "⚙"
            toolTip: qsTr("Settings (Ctrl+,)")

            onClicked: bar.settingsRequested()
        }

        BarButton {
            text: "⛶"
            toolTip: qsTr("Full screen (F)")

            onClicked: bar.fullScreenRequested()
        }
    }
}
