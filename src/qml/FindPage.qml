import QtQuick

// The official Find my earbuds page: ring each earbud, after the volume warning.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Find my earbuds")
    property bool warned: false
    readonly property var v: device.values

    function ring(side, key) {
        if (v[key]) {
            device.ring(side, false)
            return
        }
        if (warned) {
            device.ring(side, true)
            return
        }
        root.ui.confirm(qsTr("Your earbuds may be in use. Be sure to remove them from your ears before you continue. A loud sound will be played which could be uncomfortable for anyone who is wearing them."),
                        () => { root.warned = true; device.ring(side, true) }, qsTr("Volume Warning"))
    }

    Txt {
        width: parent.width
        text: qsTr("Click the play button for the earbud you wish to locate. This will trigger a sound.")
        size: 13
        color: theme.textMuted
    }
    Row {
        width: parent.width
        spacing: 12
        Repeater {
            model: [{ side: 2, key: "ringLeft", kind: "left", label: qsTr("Left") }, { side: 3, key: "ringRight", kind: "right", label: qsTr("Right") }]
            delegate: Rectangle {
                id: tile
                required property var modelData
                readonly property bool ringing: v[modelData.key] === true
                width: (parent.width - 12) / 2
                height: 250
                radius: 20
                color: theme.surface
                border.width: ringing ? 2 : 0
                border.color: theme.accent
                ProductImage {
                    x: 16; y: 16
                    width: parent.width - 32
                    height: 130
                    kind: modelData.kind
                    property real wiggle: 0
                    rotation: tile.ringing ? wiggle : 0
                    SequentialAnimation on wiggle {
                        running: tile.ringing
                        loops: Animation.Infinite
                        NumberAnimation { from: -4; to: 4; duration: 90 }
                        NumberAnimation { from: 4; to: -4; duration: 90 }
                    }
                }
                Txt { y: 156; anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; size: 15; strong: true }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 16
                    width: 52; height: 52; radius: 26
                    color: parent.ringing ? theme.accent : theme.text
                    Icon {
                        anchors.centerIn: parent
                        path: parent.parent.ringing ? "packages/nt_widget/assets/images/pause.svg" : "packages/nt_widget/assets/images/play.svg"
                        color: parent.parent.ringing ? "#FFFFFF" : theme.background
                        size: 22
                    }
                    Txt { anchors.centerIn: parent; visible: !official.available; text: parent.parent.ringing ? "■" : "▶"; color: theme.background }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.ring(modelData.side, modelData.key) }
                }
            }
        }
    }
    Txt {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        visible: v.ringLeft === true || v.ringRight === true
        text: qsTr("Ringing stops by itself after a minute.")
        size: 12
        color: theme.textMuted
    }
}
