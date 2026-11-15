import QtQuick

// The official High-quality audio page: the Bluetooth codec.
ScrollPage {
    id: root
    property var ui
    title: qsTr("High-quality audio")
    readonly property bool ldac: device.values.ldac === true

    Card {
        padding: 0
        Icon {
            width: parent.width
            height: 120
            path: theme.dark ? "assets/images/ear/ear_device_audio_quality_dark.svg" : "assets/images/ear/ear_device_audio_quality_light.svg"
            tint: false
            size: 120
        }
    }
    Card {
        spacing: 2
        Repeater {
            model: [
                { label: "AAC", sub: qsTr("Stable connection"), on: false },
                { label: "LDAC", sub: qsTr("High-resolution audio"), on: true }
            ]
            delegate: Item {
                required property var modelData
                readonly property bool selected: root.ldac === modelData.on
                width: parent.width
                height: 60
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Txt { text: modelData.label; size: 15; strong: true }
                    Txt { text: modelData.sub; size: 12; color: theme.textMuted }
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22; height: 22; radius: 11
                    color: parent.selected ? theme.text : "transparent"
                    border.width: 2
                    border.color: parent.selected ? theme.text : theme.outline
                    Rectangle { anchors.centerIn: parent; width: 8; height: 8; radius: 4; color: theme.surface; visible: parent.parent.selected }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (parent.selected)
                            return
                        if (modelData.on)
                            root.ui.guarded("ldac", true, () => device.setLdac(true))
                        else
                            device.setLdac(false)
                    }
                }
            }
        }
    }
    Txt {
        width: parent.width
        text: qsTr("LDAC audio codec will only take effect if your phone or tablet also supports the codec.")
        size: 12
        color: theme.textMuted
    }
    Txt {
        width: parent.width
        text: app.platform === "windows"
            ? qsTr("Windows uses AAC or SBC; LDAC applies to your other devices.")
            : qsTr("On Linux, LDAC plays when PipeWire offers it for these earbuds.")
        size: 12
        color: theme.textMuted
    }
}
