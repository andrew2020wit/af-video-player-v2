import QtQuick
import QtQuick.Controls

// Subtitles over the video during playback (I-22): the main ones at the top, the secondary ones at the bottom.
Item {
    id: overlay

    required property var mpv
    // Space taken at the bottom by the control panel when it is shown.
    property real bottomInset: 0

    component OverlayText: Label {
        anchors.horizontalCenter: parent.horizontalCenter
        // Wraps at 90% of the window, but at most at 1200 px: long lines are hard to read.
        width: Math.min(implicitWidth, parent.width * 0.9, 1200)
        visible: text !== ""
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        color: "#ffffff"
        leftPadding: 12
        rightPadding: 12
        topPadding: 4
        bottomPadding: 4

        background: Rectangle {
            radius: 4
            color: "#a0000000"
        }
    }

    OverlayText {
        anchors.top: parent.top
        anchors.topMargin: 24
        font.pixelSize: App.mainOverlayFontSize
        // Reading the path makes the text follow a change of the subtitle files. Nothing is looked up while the
        // overlay is hidden.
        text: {
            App.mainSubtitlesPath
            return overlay.visible ? App.mainOverlayText(overlay.mpv.position) : ""
        }
    }

    OverlayText {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24 + overlay.bottomInset
        font.pixelSize: App.secondaryOverlayFontSize
        text: {
            App.secondarySubtitlesPath
            return overlay.visible ? App.secondaryOverlayText(overlay.mpv.position) : ""
        }
    }
}
