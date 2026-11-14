import QtQuick

// A pill switch: white knob on the accent when on, outline when off.
Item {
    id: root
    property bool checked: false
    signal toggled(bool checked)
    implicitWidth: 44
    implicitHeight: 26
    opacity: enabled ? 1 : 0.4

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? theme.accent : "transparent"
        border.width: root.checked ? 0 : 1.5
        border.color: theme.textMuted
        Behavior on color { ColorAnimation { duration: 140 } }
    }
    Rectangle {
        width: root.checked ? 18 : 14
        height: width
        radius: width / 2
        y: (root.height - height) / 2
        x: root.checked ? root.width - width - 4 : 6
        color: root.checked ? theme.onAccent : theme.textMuted
        Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on width { NumberAnimation { duration: 140 } }
    }
    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
