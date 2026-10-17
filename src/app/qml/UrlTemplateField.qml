import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One URL template (I-42, I-47). It is saved only when valid (contains {text}, no whitespace); an invalid one is
// highlighted.
RowLayout {
    id: templateField

    // Saves the template, returns false when it is not valid.
    property var save: url => false
    property var reset: () => {}
    property string source: ""

    spacing: 8

    TextField {
        id: urlField

        Layout.fillWidth: true
        text: templateField.source
        placeholderText: "https://…{text}…"
        color: App.isValidUrlTemplate(text) ? palette.text : "#c62828"

        onTextEdited: templateField.save(text)
    }

    Button {
        text: qsTr("Reset to default")
        focusPolicy: Qt.NoFocus

        onClicked: {
            templateField.reset()
            urlField.text = templateField.source
        }
    }
}
