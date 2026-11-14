import QtQuick

// The rounded button: filled (primary) or outlined.
Rectangle {
    id: root
    property string text
    property string icon
    property bool primary: true
    signal clicked()
    implicitWidth: Math.max(96, row.implicitWidth + 32)
    implicitHeight: 40
    radius: height / 2
    color: primary ? theme.inverse : "transparent"
    border.width: primary ? 0 : 1.5
    border.color: theme.outline
    opacity: enabled ? (mouse.pressed ? 0.75 : 1) : 0.4

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 8
        Icon {
            visible: root.icon !== ""
            name: root.icon
            size: 16
            color: root.primary ? theme.onInverse : theme.text
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.text
            color: root.primary ? theme.onInverse : theme.text
            font.pixelSize: 13
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
