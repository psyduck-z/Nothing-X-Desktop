import QtQuick

// Model, firmware and serial number, and disconnecting.
ScrollPage {
    title: qsTr("Device information")
    readonly property var v: device.values

    Card {
        spacing: 0
        Repeater {
            model: [
                { k: qsTr("Name"), val: device.bluetoothName },
                { k: qsTr("Model"), val: device.displayName + (device.modelId ? " · " + device.modelId : "") },
                { k: qsTr("Bluetooth address"), val: device.address },
                { k: qsTr("Firmware"), val: v.firmware || "—" },
                { k: qsTr("Serial number"), val: v.serial || "—" }
            ]
            Item {
                required property var modelData
                width: parent.width
                height: 46
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.k
                    color: theme.textMuted
                    font.pixelSize: 13
                }
                TextEdit {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, parent.width * 0.6)
                    horizontalAlignment: Text.AlignRight
                    text: modelData.val
                    color: theme.text
                    font.pixelSize: 13
                    readOnly: true
                    selectByMouse: true
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }
    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        color: theme.textMuted
        font.pixelSize: 11
        text: qsTr("Firmware updates are installed by the Nothing X app on your phone.")
    }
    Row {
        spacing: 10
        PillButton {
            text: qsTr("Refresh")
            icon: "refresh"
            primary: false
            onClicked: device.refresh()
        }
        PillButton {
            text: qsTr("Disconnect")
            icon: "unlink"
            primary: false
            onClicked: device.disconnectFromDevice()
        }
    }
}
