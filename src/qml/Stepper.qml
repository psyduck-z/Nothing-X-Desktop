import QtQuick

// A value with minus and plus buttons (frequency, gain, Q of a band).
Rectangle {
    id: root
    property string label
    property string text
    signal minus()
    signal plus()
    implicitHeight: 64
    radius: 16
    color: theme.surfaceRaised
    Column {
        anchors.centerIn: parent
        spacing: 2
        SectionLabel { anchors.horizontalCenter: parent.horizontalCenter; text: root.label }
        Txt { anchors.horizontalCenter: parent.horizontalCenter; text: root.text; size: 14; strong: true; font.family: official.monoFont }
    }
    Repeater {
        model: [{ sign: "−", left: true }, { sign: "+", left: false }]
        delegate: Rectangle {
            required property var modelData
            width: 28; height: 28; radius: 14
            anchors.verticalCenter: parent.verticalCenter
            x: modelData.left ? 6 : root.width - width - 6
            color: area.containsMouse ? theme.outline : "transparent"
            Txt { anchors.centerIn: parent; text: modelData.sign; size: 18 }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: modelData.left ? root.minus() : root.plus()
                onPressAndHold: repeat.start()
                onReleased: repeat.stop()
                Timer {
                    id: repeat
                    interval: 90
                    repeat: true
                    onTriggered: modelData.left ? root.minus() : root.plus()
                }
            }
        }
    }
}
