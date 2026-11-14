import QtQuick

// Which noise modes a "Noise control" gesture cycles through.
ChoiceRow {
    title: qsTr("Cycle between")
    options: [
        { value: 22, label: qsTr("Noise cancellation, Transparency") },
        { value: 10, label: qsTr("Noise cancellation, Transparency, Off") },
        { value: 20, label: qsTr("Noise cancellation, Off") },
        { value: 21, label: qsTr("Transparency, Off") }
    ]
}
