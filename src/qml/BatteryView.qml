import QtQuick

// Left, right and case battery in Nothing's dot-matrix numerals.
Row {
    id: root
    spacing: 0
    property bool showCase: true
    readonly property var parts: {
        const p = [
            { label: qsTr("Left"), level: device.batteryLeft, charging: device.chargingLeft, inEar: device.leftInEar },
            { label: qsTr("Right"), level: device.batteryRight, charging: device.chargingRight, inEar: device.rightInEar }
        ]
        if (showCase)
            p.push({ label: qsTr("Case"), level: device.batteryCase, charging: device.chargingCase, inEar: false })
        return p
    }
    Repeater {
        model: root.parts
        delegate: Column {
            required property var modelData
            width: root.width / root.parts.length
            spacing: 4
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 2
                Dot {
                    text: modelData.level >= 0 ? modelData.level : "--"
                    size: 30
                    color: modelData.level >= 0 && modelData.level <= 20 && !modelData.charging ? theme.accent : theme.text
                }
                Dot { text: "%"; size: 16; color: theme.textMuted; anchors.bottom: parent.bottom; anchors.bottomMargin: 4; visible: modelData.level >= 0 }
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6
                Rectangle {
                    width: 6; height: 6; radius: 3
                    anchors.verticalCenter: parent.verticalCenter
                    visible: modelData.charging
                    color: theme.charging
                }
                SectionLabel { text: modelData.label + (modelData.charging ? " · " + qsTr("Charging") : "") }
            }
        }
    }
}
