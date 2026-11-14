import QtQuick

// Find my earbuds: play a loud tone from one earbud.
ScrollPage {
    title: qsTr("Find my earbuds")
    readonly property var v: device.values

    Card {
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.accent
            font.pixelSize: 12
            text: qsTr("Take the earbuds out of your ears first: the tone is loud.")
        }
    }
    Row {
        width: parent.width
        spacing: 12
        Repeater {
            model: [{ side: 2, label: qsTr("Left"), icon: "budleft", key: "ringingLeft" },
                    { side: 3, label: qsTr("Right"), icon: "budright", key: "ringingRight" }]
            Rectangle {
                required property var modelData
                readonly property bool ringing: v[modelData.key] === true
                width: (parent.width - 12) / 2
                height: 170
                radius: 18
                color: ringing ? theme.inverse : theme.surface
                Column {
                    anchors.centerIn: parent
                    spacing: 12
                    Icon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        name: modelData.icon
                        size: 40
                        color: ringing ? theme.onInverse : theme.text
                        SequentialAnimation on scale {
                            running: ringing
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.12; duration: 260 }
                            NumberAnimation { to: 1.0; duration: 260 }
                        }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.label
                        color: ringing ? theme.onInverse : theme.text
                        font.pixelSize: 14
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: ringing ? qsTr("Tap to stop") : qsTr("Tap to play sound")
                        color: ringing ? theme.onInverse : theme.textMuted
                        font.pixelSize: 11
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: device.ring(modelData.side, !ringing)
                }
            }
        }
    }
}
