pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// All cues of the main subtitles with the secondary ones (F-42 - F-47), opened by Space; the video is paused
// while it is shown.
Rectangle {
    id: list

    required property var mpv
    // The cue playback is at (F-45).
    readonly property int currentIndex: App.subtitles.count > 0 ? App.subtitles.indexAt(mpv.position) : -1
    // The cue that was current when the list was closed last time (F-45).
    property int previousIndex: -1
    // Marked by a right click (F-45): one mark, in memory only.
    property int markedIndex: -1
    // Chapter headings by the row they are shown above (I-89): {row: [{label, time}]}; the row count is the end
    // of the list, for chapters after the last cue start.
    readonly property var chapterRows: {
        const rows = {}
        const chapters = mpv.chapters

        if (App.subtitles.count === 0) {
            return rows
        }

        for (let i = 0; i < chapters.length; ++i) {
            const title = chapters[i].title.trim()
            const label = title !== "" ? qsTr("Chapter %1: %2").arg(i + 1).arg(title) : qsTr("Chapter %1").arg(i + 1)
            const row = App.subtitles.indexFrom(chapters[i].time)

            if (!rows[row]) {
                rows[row] = []
            }

            rows[row].push({label: label, time: chapters[i].time})
        }

        return rows
    }
    // The next video in the folder, set when the video has played to the end (I-16).
    property string nextVideoPath: ""

    signal subtitlesDialogRequested()
    signal exportRequested()
    signal nextVideoRequested()
    signal settingsRequested()
    // Close the list and continue playback.
    signal closeRequested()

    function reset() {
        previousIndex = -1
        markedIndex = -1
    }

    function scrollBy(pixels) {
        const minimum = listView.originY
        const maximum = Math.max(minimum, listView.originY + listView.contentHeight - listView.height)
        listView.contentY = Math.max(minimum, Math.min(maximum, listView.contentY + pixels))
    }

    function scrollToBeginning() {
        listView.positionViewAtBeginning()
    }

    function scrollToEnd() {
        listView.positionViewAtEnd()
    }

    function scrollToCurrent() {
        if (currentIndex >= 0) {
            listView.positionViewAtIndex(currentIndex, ListView.Center)
        }
    }

    // A click on a chapter heading (I-89): seek to the chapter start and continue playback, as a cue time (F-46).
    function goToChapter(seconds) {
        mpv.seek(seconds)
        closeRequested()
    }

    color: "#ffffff"

    onVisibleChanged: {
        if (visible) {
            Qt.callLater(scrollToCurrent)
        } else {
            previousIndex = currentIndex
        }
    }

    // Subtitles chosen while the list is open.
    Connections {
        target: App.subtitles

        function onCountChanged() {
            if (list.visible) {
                Qt.callLater(list.scrollToCurrent)
            }
        }
    }

    // Clicks and the wheel outside the rows do not reach the video below: no pause toggle, no volume change.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons

        onWheel: wheel => wheel.accepted = true
    }

    ListView {
        id: listView

        anchors.fill: parent
        anchors.topMargin: nextButton.visible ? nextButton.height + 16 : 8
        model: App.subtitles
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: 2000
        visible: App.subtitles.count > 0

        delegate: SubtitleRow {
            id: row

            isCurrent: row.index === list.currentIndex
            isPrevious: row.index === list.previousIndex && row.index !== list.currentIndex
            isMarked: row.index === list.markedIndex
            chapters: list.chapterRows[row.index] || []

            onTimeClicked: {
                // F-46: seek to the cue start and continue playback.
                list.mpv.seek(App.subtitles.startSeconds(row.index))
                list.closeRequested()
            }
            onMarkToggled: list.markedIndex = list.markedIndex === row.index ? -1 : row.index
            onChapterClicked: seconds => list.goToChapter(seconds)
        }

        // Chapters after the last cue start (I-89).
        footer: Column {
            width: listView.width - 16

            Repeater {
                model: list.chapterRows[App.subtitles.count] || []

                ChapterHeading {
                    required property var modelData

                    width: listView.width - 16
                    text: modelData.label

                    onClicked: list.goToChapter(modelData.time)
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
        }
    }

    // The video has played to the end (I-16).
    Button {
        id: nextButton

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 8
        visible: list.nextVideoPath !== ""
        text: qsTr("Next: %1").arg(App.fileName(list.nextVideoPath))
        focusPolicy: Qt.NoFocus

        onClicked: list.nextVideoRequested()

        ToolTip.visible: hovered
        ToolTip.text: qsTr("Next file in the folder (N)")
    }

    // No main subtitles (I-56).
    Column {
        anchors.centerIn: parent
        spacing: 16
        visible: App.subtitles.count === 0

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No subtitles")
            font.pixelSize: 24
            color: "#333333"
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            Button {
                text: qsTr("Choose files…")
                focusPolicy: Qt.NoFocus

                onClicked: list.subtitlesDialogRequested()
            }

            Button {
                text: qsTr("Export embedded…")
                focusPolicy: Qt.NoFocus
                visible: App.embeddedTextTrackCount > 0

                onClicked: list.exportRequested()
            }
        }
    }

    // The top right corner: closing the list (F-42), the settings (I-50) and the subtitles (I-57).
    Column {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        anchors.rightMargin: 24
        spacing: 4

        // Close the list and continue playback (F-42).
        ToolButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "✕"
            font.pixelSize: 22
            focusPolicy: Qt.NoFocus

            onClicked: list.closeRequested()

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Continue (Space)")
        }

        // The settings cover the list; it is still open when they are closed.
        ToolButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⚙"
            font.pixelSize: 22
            // A monochrome glyph in the text colour; the default font falls back to a pale colour emoji on Windows.
            font.family: "Segoe UI Symbol"
            focusPolicy: Qt.NoFocus

            onClicked: list.settingsRequested()

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Settings (Ctrl+,)")
        }

        // The "Subtitles" dialog, as the CC button of the control panel (I-57).
        ToolButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "CC"
            font.pixelSize: 15
            font.bold: true
            focusPolicy: Qt.NoFocus

            onClicked: list.subtitlesDialogRequested()

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Subtitles")
        }
    }
}
