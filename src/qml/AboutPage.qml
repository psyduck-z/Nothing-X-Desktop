import QtQuick

// The official Device details page.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Device details")
    readonly property var v: device.values

    Card {
        spacing: 4
        Repeater {
            model: [
                { t: qsTr("Device name"), d: device.bluetoothName },
                { t: qsTr("Model"), d: device.displayName + (device.modelId !== "" ? " (" + device.modelId + ")" : "") },
                { t: qsTr("Colour"), d: device.colorId === "02" ? qsTr("White") : device.colorId === "01" ? qsTr("Black") : "" },
                { t: qsTr("Serial number"), d: v.serial || "" },
                { t: qsTr("Bluetooth address"), d: device.address },
                { t: qsTr("Firmware version"), d: v.firmware || "" },
                { t: qsTr("Case firmware"), d: v.caseFirmware || "" }
            ]
            delegate: ListRow {
                required property var modelData
                visible: modelData.d !== ""
                title: modelData.t
                subtitle: modelData.d
            }
        }
    }
    Txt {
        width: parent.width
        text: qsTr("Firmware updates are installed by the Nothing X phone app.")
        size: 12
        color: theme.textMuted
    }
}
