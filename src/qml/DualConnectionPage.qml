import QtQuick

// The official Dual Connection page: the switch and the remembered devices.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Dual Connection")
    readonly property bool on: device.values.dual === true
    Component.onCompleted: device.refreshPairedDevices()

    Card {
        ListRow {
            icon: "assets/images/ear/ear_dual_connection.svg"
            title: qsTr("Dual Connection")
            subtitle: qsTr("You can connect up to 2 devices with your earbuds at the same time. This feature will impact battery life.")
            hasSwitch: true
            checked: root.on
            onToggled: (v) => device.features.dualReboot
                ? root.ui.confirm(qsTr("Turning on/off dual connection requires rebooting the earbuds"), () => device.setDualConnection(v), qsTr("Earbuds reboot required"))
                : device.setDualConnection(v)
        }
    }
    Card {
        visible: root.on
        spacing: 2
        SectionLabel { text: qsTr("My devices") }
        Repeater {
            model: device.pairedDevices
            delegate: Item {
                required property var modelData
                width: parent.width
                height: 56
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 130
                    spacing: 2
                    Txt { width: parent.width; text: modelData.name || modelData.address; size: 15 }
                    Row {
                        spacing: 6
                        Rectangle { width: 6; height: 6; radius: 3; anchors.verticalCenter: parent.verticalCenter; color: modelData.connected ? theme.good : theme.textDim }
                        Txt {
                            text: modelData.thisHost ? qsTr("Current") : modelData.connected ? qsTr("Connected") : qsTr("Not connected")
                            size: 12
                            color: theme.textMuted
                        }
                    }
                }
                Pill {
                    visible: !modelData.thisHost
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    implicitHeight: 34
                    text: modelData.connected ? qsTr("Disconnect") : qsTr("Connect")
                    onClicked: modelData.connected
                        ? root.ui.confirm(qsTr("This action will disconnect the Bluetooth connection with %1.").arg(modelData.name),
                                          () => device.connectPairedDevice(modelData.address, false), qsTr("Disconnect device"))
                        : device.connectPairedDevice(modelData.address, true)
                }
            }
        }
        Txt {
            visible: device.pairedDevices.length === 0
            width: parent.width
            text: qsTr("No other devices remembered yet.")
            size: 13
            color: theme.textMuted
        }
    }
}
