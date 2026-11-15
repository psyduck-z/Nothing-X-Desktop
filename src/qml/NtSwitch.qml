import QtQuick

// Nothing's switch: a pill track, a round knob, red when on.
Item {
    id: root
    property bool checked: false
    property bool enabled2: true
    signal toggled(bool value)
    implicitWidth: 44
    implicitHeight: 26
    opacity: enabled2 ? 1 : 0.4
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? theme.accent : theme.surfaceRaised
        Behavior on color { ColorAnimation { duration: 140 } }
        Rectangle {
            width: 20
            height: 20
            radius: 10
            y: 3
            x: root.checked ? parent.width - width - 3 : 3
            color: root.checked ? "#FFFFFF" : theme.textMuted
            Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }
    }
    MouseArea {
        anchors.fill: parent
        anchors.margins: -6
        cursorShape: Qt.PointingHandCursor
        enabled: root.enabled2
        onClicked: root.toggled(!root.checked)
    }
}
