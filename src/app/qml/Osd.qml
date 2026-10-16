import QtQuick
import QtQuick.Controls

// On-screen message (I-60): a short text in the corner of the video for ~1.5 s.
Rectangle {
    id: osd

    function show(message) {
        label.text = message
        opacity = 1
        hideTimer.restart()
    }

    width: label.implicitWidth + 28
    height: label.implicitHeight + 16
    radius: 6
    color: "#b0000000"
    opacity: 0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: 200
        }
    }

    Label {
        id: label

        anchors.centerIn: parent
        color: "#ffffff"
        font.pixelSize: 18
    }

    Timer {
        id: hideTimer

        interval: 1500

        onTriggered: osd.opacity = 0
    }
}
