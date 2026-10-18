import QtQuick
import QtQuick.Controls

// A chapter heading in the subtitle list (I-89): "Chapter N: title" in the main subtitle font, centred; a click
// goes to the chapter start.
Item {
    id: heading

    required property string text

    signal clicked()

    implicitHeight: label.implicitHeight + 16

    Label {
        id: label

        anchors.centerIn: parent
        width: Math.min(implicitWidth, heading.width - 32)
        text: heading.text
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: "#111111"
        font.pixelSize: App.mainSubtitleFontSize
        font.underline: hoverHandler.hovered

        HoverHandler {
            id: hoverHandler

            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: heading.clicked()
        }

        ToolTip.visible: hoverHandler.hovered
        ToolTip.text: qsTr("Go to the chapter start")
    }
}
