import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Start window (I-56): open a video and the history (I-11).
Rectangle {
    id: startPage

    // Expanded series by folder (I-76), for this run only.
    property var expandedFolders: ({})
    // The history lists folders on disk: it is read when the page is shown (files may have appeared or disappeared
    // since) and on changes while it is shown, not on every save of the playback position.
    readonly property var history: visible ? App.history : []

    signal openFileRequested()
    signal settingsRequested()
    signal historyItemClicked(string path)

    function toggleFolder(folder) {
        const folders = Object.assign({}, expandedFolders)
        folders[folder] = folders[folder] !== true
        expandedFolders = folders
    }

    color: palette.window

    ToolButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
        text: "⚙"
        font.pixelSize: 24
        // A monochrome glyph in the text colour; the default font falls back to a pale colour emoji on Windows.
        font.family: "Segoe UI Symbol"
        focusPolicy: Qt.NoFocus

        onClicked: startPage.settingsRequested()

        ToolTip.visible: hovered
        ToolTip.text: qsTr("Settings (Ctrl+,)")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 48
        anchors.bottomMargin: 24
        anchors.leftMargin: Math.max(24, (parent.width - 900) / 2)
        anchors.rightMargin: Math.max(24, (parent.width - 900) / 2)
        spacing: 12

        Label {
            text: "AF Video Player"
            font.pixelSize: 30
            font.bold: true
        }

        RowLayout {
            spacing: 16

            Button {
                text: qsTr("Open video…")
                font.pixelSize: 16
                focusPolicy: Qt.NoFocus
                highlighted: true

                onClicked: startPage.openFileRequested()
            }

            Label {
                text: qsTr("Ctrl+O, or drop a file onto the window")
                color: palette.placeholderText
            }
        }

        Label {
            Layout.topMargin: 24
            text: qsTr("History")
            font.pixelSize: 20
            font.bold: true
        }

        Label {
            visible: startPage.history.length === 0
            text: qsTr("Opened files will appear here.")
            color: palette.placeholderText
        }

        ListView {
            id: historyView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: startPage.history
            boundsBehavior: Flickable.StopAtBounds
            spacing: 2

            // A folder (I-76): a series gets a header with the progress and "Continue", its files are shown
            // when it is expanded; files of other folders are shown as they are.
            delegate: Column {
                id: group

                required property var modelData
                readonly property bool expanded: startPage.expandedFolders[group.modelData.folder] === true

                width: ListView.view.width

                SeriesHeader {
                    width: parent.width
                    visible: group.modelData.isSeries
                    series: group.modelData
                    expanded: group.expanded

                    onClicked: startPage.toggleFolder(group.modelData.folder)
                    onContinueRequested: startPage.historyItemClicked(group.modelData.continuePath)
                    onRemoveRequested: removeSeriesDialog.ask(group.modelData)
                }

                Repeater {
                    model: !group.modelData.isSeries || group.expanded ? group.modelData.entries : []

                    delegate: HistoryRow {
                        required property var modelData

                        width: group.width
                        entry: modelData
                        inSeries: group.modelData.isSeries

                        onClicked: startPage.historyItemClicked(modelData.path)
                        onRemoveRequested: App.removeFromHistory([modelData.path])
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }

    // A series is removed with all its files, so it is confirmed (I-15).
    Dialog {
        id: removeSeriesDialog

        property var series: ({})

        function ask(seriesToRemove) {
            series = seriesToRemove
            open()
        }

        anchors.centerIn: parent
        modal: true
        title: qsTr("Remove the series from history?")
        footer: DialogButtonBox {
            Button {
                text: qsTr("Remove")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }

            Button {
                text: qsTr("Cancel")
                focusPolicy: Qt.NoFocus
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }

        onAccepted: App.removeFromHistory(series.entries.map(entry => entry.path))

        Label {
            text: qsTr("Positions, subtitles and audio tracks of %n file(s) in “%1” will be forgotten.", "",
                       removeSeriesDialog.series.entries ? removeSeriesDialog.series.entries.length : 0)
                  .arg(removeSeriesDialog.series.folderName)
        }
    }

    // Outline of a hovered history item; the pointing hand cursor comes from HandCursorFilter.
    component HoverOutline: Rectangle {
        required property bool shown

        anchors.fill: parent
        visible: shown
        color: "transparent"
        border.width: 2
        border.color: palette.highlight
    }

    // Removes an item from the history (I-15); shown while the item is hovered.
    component RemoveButton: ToolButton {
        required property bool shown

        text: "✕"
        focusPolicy: Qt.NoFocus
        opacity: shown ? 1 : 0
        enabled: shown

        ToolTip.visible: hovered
        ToolTip.text: qsTr("Remove from history")
    }

    // A file of the history: a click opens it.
    component HistoryRow: ItemDelegate {
        id: row

        property var entry: ({})
        // Files of a series are indented under its header and show no folder.
        property bool inSeries: false

        signal removeRequested()

        focusPolicy: Qt.NoFocus
        leftPadding: inSeries ? 40 : 12

        // Unlike "hovered", stays on over the remove button; the whole row is clickable.
        HoverHandler {
            id: rowHover

            cursorShape: Qt.PointingHandCursor
        }

        HoverOutline {
            shown: rowHover.hovered
        }

        contentItem: RowLayout {
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: row.entry.name
                    font.pixelSize: row.inSeries ? 15 : 16
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    visible: !row.inSeries
                    text: row.entry.folder
                    color: palette.placeholderText
                    elide: Text.ElideMiddle
                }
            }

            // Watched files (I-76) get a check mark instead of the share.
            Label {
                Layout.preferredWidth: 56
                horizontalAlignment: Text.AlignRight
                text: row.entry.watched ? "✓" : row.entry.progress >= 0 ? Math.round(row.entry.progress * 100) + "%" : ""
                color: row.entry.watched ? "#166534" : palette.windowText
                font.bold: row.entry.watched
            }

            Label {
                Layout.preferredWidth: 120
                horizontalAlignment: Text.AlignRight
                text: row.entry.lastOpened
                color: palette.placeholderText
            }

            RemoveButton {
                shown: rowHover.hovered

                onClicked: row.removeRequested()
            }
        }
    }

    // A series (I-76): folder, watched episodes, "Continue" with the next episode. A click expands it.
    component SeriesHeader: ItemDelegate {
        id: header

        property var series: ({})
        property bool expanded: false

        signal continueRequested()
        signal removeRequested()

        focusPolicy: Qt.NoFocus

        HoverHandler {
            id: headerHover

            cursorShape: Qt.PointingHandCursor
        }

        HoverOutline {
            shown: headerHover.hovered
        }

        contentItem: RowLayout {
            spacing: 16

            Label {
                text: header.expanded ? "▾" : "▸"
                font.pixelSize: 16
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: header.series.folderName
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("%1 of %2 watched").arg(header.series.watchedCount).arg(header.series.videoCount)
                          + " · " + header.series.folder
                    color: palette.placeholderText
                    elide: Text.ElideMiddle
                }
            }

            Button {
                id: continueButton

                Layout.maximumWidth: 320
                visible: header.series.continuePath !== ""
                text: qsTr("Continue: %1").arg(header.series.continueName)
                focusPolicy: Qt.NoFocus

                onClicked: header.continueRequested()

                // White on blue in every state (SPEC 3.5).
                background: Rectangle {
                    implicitHeight: 32
                    radius: 3
                    color: continueButton.down ? "#1e3a8a" : continueButton.hovered ? "#1e40af" : "#1d4ed8"
                }

                contentItem: Label {
                    text: continueButton.text
                    color: "#ffffff"
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                ToolTip.visible: hovered
                ToolTip.text: header.series.continueName
            }

            Label {
                Layout.preferredWidth: 120
                horizontalAlignment: Text.AlignRight
                text: header.series.lastOpened
                color: palette.placeholderText
            }

            RemoveButton {
                shown: headerHover.hovered

                onClicked: header.removeRequested()
            }
        }
    }
}
