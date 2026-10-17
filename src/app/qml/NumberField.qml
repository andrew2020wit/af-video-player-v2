import QtQuick
import QtQuick.Controls

// An editable whole number in [from, to]: only digits can be typed, the value is shown without group separators
// (the locale's separator may be a space). An empty or unfinished value is not applied.
SpinBox {
    id: numberField

    editable: true
    inputMethodHints: Qt.ImhDigitsOnly
    validator: RegularExpressionValidator {
        regularExpression: /[0-9]{0,6}/
    }
    textFromValue: value => String(value)
    valueFromText: text => {
        const number = parseInt(text, 10)

        return isNaN(number) ? numberField.value : Math.max(numberField.from, Math.min(numberField.to, number))
    }
}
