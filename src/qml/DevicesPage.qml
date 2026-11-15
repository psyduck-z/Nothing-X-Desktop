import QtQuick

// Choose the earbuds: the Nothing and CMF devices paired with this computer.
ScrollPage {
    id: root
    property var ui
    title: qsTr("My devices")
    Component.onCompleted: scanner.refresh()

    Card {
        visible: !scanner.adapterPoweredOn
        Txt { width: parent.width; text: scanner.adapterAvailable ? qsTr("Bluetooth is off.") : qsTr("No Bluetooth adapter found."); size: 15 }
        Pill { visible: scanner.adapterAvailable; text: qsTr("Turn on Bluetooth"); primary: true; onClicked: scanner.powerOnAdapter() }
    }

    Card {
        visible: scanner.adapterPoweredOn
        spacing: 2
        SectionLabel { text: scanner.scanning ? qsTr("Looking for your earbuds...") : qsTr("Paired with this computer") }
        Repeater {
            model: scanner.devices
            delegate: Item {
                required property var modelData
                readonly property bool current: modelData.address === device.address && device.state !== 0
                width: parent.width
                height: 60
                Icon { id: icon; path: "assets/images/device/device_ear_icon.svg"; size: 24; anchors.verticalCenter: parent.verticalCenter }
                Column {
                    anchors.left: icon.visible ? icon.right : parent.left
                    anchors.leftMargin: icon.visible ? 14 : 0
                    anchors.right: pill.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Txt { width: parent.width; text: modelData.name; size: 15 }
                    Row {
                        spacing: 6
                        Rectangle { width: 6; height: 6; radius: 3; anchors.verticalCenter: parent.verticalCenter; color: parent.parent.parent.current && device.state === 2 ? theme.good : modelData.connected ? theme.charging : theme.textDim }
                        Txt {
                            size: 12
                            color: theme.textMuted
                            text: parent.parent.parent.current ? (device.state === 2 ? qsTr("In use") : qsTr("Connecting...")) : modelData.connected ? qsTr("Connected to this computer") : qsTr("Not connected")
                        }
                    }
                }
                Pill {
                    id: pill
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    implicitHeight: 34
                    text: parent.current ? qsTr("Disconnect") : qsTr("Use")
                    primary: !parent.current
                    onClicked: {
                        if (parent.current) {
                            device.disconnectFromDevice()
                        } else {
                            device.connectToDevice(modelData.address, modelData.name)
                            root.back()
                        }
                    }
                }
            }
        }
        Txt {
            visible: scanner.devices.length === 0 && !scanner.scanning
            width: parent.width
            text: qsTr("No Nothing earbuds are paired yet. Open the case, hold its button for 2 seconds, then pair them in your Bluetooth settings.")
            size: 13
            color: theme.textMuted
        }
        Row {
            spacing: 10
            topPadding: 6
            Pill { text: qsTr("Bluetooth settings"); onClicked: app.openBluetoothSettings() }
            Pill { text: qsTr("Refresh"); enabled2: !scanner.scanning; onClicked: scanner.refresh() }
        }
    }
    Txt {
        visible: scanner.error !== ""
        width: parent.width
        text: scanner.error
        size: 12
        color: theme.accent
    }
}
