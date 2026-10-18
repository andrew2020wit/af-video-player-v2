pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A row of the subtitle list (F-43): secondary text, start time, main text, translation button on hover.
Item {
    id: row

    required property int index
    required property string timeText
    required property string text
    required property string secondaryText
    required property real pauseFraction
    property bool isCurrent: false
    property bool isPrevious: false
    property bool isMarked: false
    // Chapters that start after the previous cue and not later than this one (I-89): [{label, time}].
    property var chapters: []

    signal timeClicked()
    signal markToggled()
    signal chapterClicked(real seconds)

    // Cue text: a click looks up the word under the cursor, a drag selects text and looks up the selection (I-59).
    component CueText: TextEdit {
        id: cueText

        Layout.fillWidth: true
        readOnly: true
        selectByMouse: false
        wrapMode: TextEdit.Wrap
        textFormat: TextEdit.PlainText
        selectionColor: "#93c5fd"
        selectedTextColor: "#000000"

        MouseArea {
            property int pressPosition: -1
            property bool dragged: false

            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor

            onPressed: mouse => {
                pressPosition = cueText.positionAt(mouse.x, mouse.y)
                dragged = false
                cueText.deselect()
            }
            onPositionChanged: mouse => {
                const position = cueText.positionAt(mouse.x, mouse.y)

                if (position !== pressPosition) {
                    dragged = true
                    cueText.select(pressPosition, position)
                }
            }
            onReleased: {
                if (dragged && cueText.selectedText !== "") {
                    App.lookupSelection(cueText.selectedText)
                } else {
                    App.lookupWord(cueText.text, pressPosition)
                }
            }
        }
    }

    width: ListView.view ? ListView.view.width - 16 : 0
    height: headings.height + content.implicitHeight + 12

    // Chapter headings above the cue (I-89), outside its highlight.
    Column {
        id: headings

        width: row.width

        Repeater {
            model: row.chapters

            ChapterHeading {
                required property var modelData

                width: headings.width
                text: modelData.label

                onClicked: row.chapterClicked(modelData.time)
            }
        }
    }

    Rectangle {
        y: headings.height
        width: row.width
        height: row.height - headings.height
        color: row.isCurrent ? "#dbeafe" : row.isPrevious ? "#fff4c2" : "transparent"
    }

    HoverHandler {
        id: hoverHandler
    }

    // Right click marks the cue (F-45).
    TapHandler {
        acceptedButtons: Qt.RightButton

        onTapped: row.markToggled()
    }

    ColumnLayout {
        id: content

        x: 16
        y: headings.height + 6
        width: row.width - 32
        spacing: 4

        // Pause before the cue (F-47).
        Rectangle {
            Layout.preferredWidth: content.width * row.pauseFraction
            Layout.preferredHeight: 4
            Layout.bottomMargin: 6
            visible: row.pauseFraction > 0
            radius: 2
            color: "#6b7280"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            // Secondary and main texts side by side, 40% and 60% of the space left by the time and the button: the
            // translation on the left, the time and the original on the right. The secondary column is kept for
            // rows without secondary text, so the columns line up across the list.
            CueText {
                Layout.alignment: Qt.AlignTop
                // Aligns the first line's baseline with the main text.
                Layout.topMargin: mainText.baselineOffset - baselineOffset
                Layout.preferredWidth: 1
                Layout.horizontalStretchFactor: 2
                visible: App.secondarySubtitlesPath !== ""
                text: row.secondaryText
                color: "#146c2e"
                font.pixelSize: App.secondarySubtitleFontSize
            }

            Label {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 6
                Layout.preferredWidth: 72
                text: row.timeText
                color: "#5b6270"
                font.pixelSize: 15
                // Underlined under the mouse: the time is clickable (F-46).
                font.underline: timeMouseArea.containsMouse

                MouseArea {
                    id: timeMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onClicked: row.timeClicked()
                }
            }

            CueText {
                id: mainText

                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 1
                Layout.horizontalStretchFactor: 3
                text: row.text
                color: "#111111"
                font.pixelSize: App.mainSubtitleFontSize
                font.bold: row.isMarked
            }

            // Translation of the cue in the browser (I-47).
            ToolButton {
                Layout.alignment: Qt.AlignTop
                text: "G"
                font.bold: true
                focusPolicy: Qt.NoFocus
                opacity: hoverHandler.hovered ? 1 : 0
                enabled: hoverHandler.hovered

                onClicked: App.translate(row.text)

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Translate in the browser")
            }
        }
    }
}
