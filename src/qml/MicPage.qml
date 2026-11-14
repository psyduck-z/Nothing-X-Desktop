import QtQuick

// Super Mic, microphone mode, call transparency and walkie-talkie mode.
ScrollPage {
    title: qsTr("Microphone & calls")
    readonly property var v: device.values

    Card {
        ToggleRow {
            visible: device.features.includes("superMic")
            icon: "mic"
            title: qsTr("Super Mic")
            subtitle: qsTr("Uses the case as a microphone for clearer calls and voice notes. Double press the Talk button to toggle.")
            checked: v.superMic === true
            onToggled: (on) => device.setSuperMic(on)
        }
        Column {
            visible: device.features.includes("micMode") && v.superMic === true
            width: parent.width
            spacing: 8
            Text { text: qsTr("Mic mode"); color: theme.text; font.pixelSize: 14 }
            Segmented {
                options: [{ value: 0, label: qsTr("Natural") }, { value: 1, label: qsTr("Voice focus") }]
                current: v.micMode || 0
                onPicked: (m) => device.setMicMode(m)
            }
        }
    }
    Card {
        ToggleRow {
            visible: device.features.includes("callTransparency")
            icon: "transparency"
            title: qsTr("Call transparency")
            subtitle: qsTr("Hear your own voice naturally during calls")
            checked: v.callTransparency === true
            onToggled: (on) => device.setCallTransparency(on)
        }
        ToggleRow {
            visible: device.features.includes("walkieTalkie")
            icon: "assistant"
            title: qsTr("Walkie-talkie mode")
            subtitle: qsTr("Push-to-talk voice messages with the Talk button")
            checked: v.walkieTalkie === true
            onToggled: (on) => device.setWalkieTalkie(on)
        }
    }
}
