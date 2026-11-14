import QtQuick

// Dual connection: stay connected to two devices, and pick which ones.
ScrollPage {
    title: qsTr("Dual connection")
    readonly property var v: device.values

    Card {
        ToggleRow {
            icon: "devices"
            title: qsTr("Dual connection")
            subtitle: qsTr("Connect to two devices at once, for example this computer and your phone")
            checked: v.dualConnection === true
            onToggled: (on) => device.setDualConnection(on)
        }
    }

    SectionLabel { text: qsTr("Remembered devices") }
    Card {
        spacing: 4
        Repeater {
            model: device.pairedDevices
            Item {
                required property var modelData
                width: parent.width
                height: 52
                Column {
                    anchors.left: parent.left
                    anchors.right: action.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: modelData.name + (modelData.thisHost ? qsTr(" (this computer)") : "")
                        color: theme.text
                        font.pixelSize: 14
                        elide: Text.ElideRight
                    }
                    Text {
                        text: modelData.connected ? qsTr("Connected") : qsTr("Not connected")
                        color: modelData.connected ? theme.accent : theme.textMuted
                        font.pixelSize: 11
                    }
                }
                PillButton {
                    id: action
                    visible: !modelData.thisHost
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    implicitHeight: 32
                    primary: false
                    text: modelData.connected ? qsTr("Disconnect") : qsTr("Connect")
                    onClicked: device.connectPairedDevice(modelData.address, !modelData.connected)
                }
            }
        }
        Text {
            visible: device.pairedDevices.length === 0
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.textMuted
            font.pixelSize: 13
            text: qsTr("No devices reported yet.")
        }
    }
    PillButton {
        text: qsTr("Refresh")
        icon: "refresh"
        primary: false
        onClicked: device.refreshPairedDevices()
    }
}
