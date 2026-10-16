import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Language choice from the ISO 639-1 list with a filter (I-50).
Button {
    id: picker

    // ISO 639-1 code; empty means emptyText (allowed only with allowEmpty).
    property string code: ""
    property bool allowEmpty: false
    property string emptyText: ""

    signal picked(string code)

    text: code !== "" ? App.languageLabel(code) : emptyText
    focusPolicy: Qt.NoFocus

    onClicked: {
        filter.text = ""
        popup.open()
        filter.forceActiveFocus()
    }

    Popup {
        id: popup

        y: picker.height
        width: Math.max(picker.width, 320)
        height: 360
        modal: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            TextField {
                id: filter

                Layout.fillWidth: true
                placeholderText: qsTr("Search: English, en, eng…")
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: {
                    const query = filter.text.trim().toLowerCase()
                    const languages = App.languages.filter(language => query === "" || language.search.includes(query))

                    if (picker.allowEmpty && query === "") {
                        return [{ code: "", label: picker.emptyText }].concat(languages)
                    }

                    return languages
                }

                delegate: ItemDelegate {
                    required property var modelData

                    width: ListView.view.width
                    text: modelData.label
                    highlighted: modelData.code === picker.code
                    focusPolicy: Qt.NoFocus

                    onClicked: {
                        popup.close()
                        picker.picked(modelData.code)
                    }
                }

                ScrollBar.vertical: ScrollBar {}
            }
        }
    }
}
