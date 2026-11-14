import QtQuick

// Pick which paired Nothing device to control.
ScrollPage {
    objectName: "devices"
    id: page
    title: qsTr("Devices")

    Component.onCompleted: scanner.refresh()

    Card {
        visible: !scanner.adapterAvailable || !scanner.adapterPoweredOn
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.text
            font.pixelSize: 13
            text: !scanner.adapterAvailable ? qsTr("No Bluetooth adapter was found on this computer.")
                                            : qsTr("Bluetooth is turned off.")
        }
        PillButton {
            visible: scanner.adapterAvailable
            text: qsTr("Turn on Bluetooth")
            onClicked: scanner.powerOnAdapter()
        }
    }

    SectionLabel { text: qsTr("Paired Nothing devices") }

    Card {
        spacing: 4
        Repeater {
            model: scanner.devices
            Item {
                required property var modelData
                readonly property bool active: device.address === modelData.address && device.state !== 0
                width: parent.width
                height: 54
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -4
                    radius: 12
                    color: theme.text
                    opacity: rowMouse.containsMouse ? 0.06 : 0
                }
                Icon {
                    id: budIcon
                    name: "budright"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Column {
                    anchors.left: budIcon.right
                    anchors.leftMargin: 12
                    anchors.right: status.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: modelData.name
                        color: theme.text
                        font.pixelSize: 14
                        elide: Text.ElideRight
                    }
                    Text {
                        text: modelData.address
                        color: theme.textMuted
                        font.pixelSize: 11
                        font.family: "monospace"
                    }
                }
                Text {
                    id: status
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    color: active ? theme.accent : theme.textMuted
                    font.pixelSize: 12
                    text: active ? (device.state === 2 ? qsTr("In use") : qsTr("Connecting…"))
                                 : modelData.connected ? qsTr("Connected") : qsTr("Not connected")
                }
                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        device.connectToDevice(modelData.address, modelData.name)
                        page.back()
                    }
                }
            }
        }
        Text {
            visible: scanner.devices.length === 0
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.textMuted
            font.pixelSize: 13
            text: scanner.scanning ? qsTr("Looking for paired devices…")
                                   : qsTr("No Nothing earbuds are paired with this computer yet. Put them in pairing mode (open the case and hold its button for 2 seconds), then pair them in your Bluetooth settings.")
        }
    }

    Row {
        spacing: 10
        PillButton {
            text: scanner.scanning ? qsTr("Searching…") : qsTr("Refresh")
            icon: "refresh"
            primary: false
            enabled: !scanner.scanning
            onClicked: scanner.refresh()
        }
        PillButton {
            text: qsTr("Bluetooth settings")
            icon: "bluetooth"
            primary: false
            onClicked: app.openBluetoothSettings()
        }
    }

    Text {
        visible: scanner.error !== ""
        width: parent.width
        wrapMode: Text.WordWrap
        color: theme.accent
        font.pixelSize: 12
        text: scanner.error
    }
}
