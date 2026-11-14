import QtQuick

// A square noise-control choice: icon above label, filled when active.
Rectangle {
    id: root
    property string icon
    property string label
    property bool active
    signal clicked()
    height: 84
    radius: 16
    color: active ? theme.inverse : theme.surfaceRaised
    Behavior on color { ColorAnimation { duration: 140 } }

    Column {
        anchors.centerIn: parent
        width: parent.width - 8
        spacing: 6
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.icon
            size: 24
            color: root.active ? theme.onInverse : theme.text
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.label
            color: root.active ? theme.onInverse : theme.text
            font.pixelSize: 11
            lineHeight: 0.95
            wrapMode: Text.WordWrap
        }
    }
    Rectangle {
        visible: root.active
        width: 6; height: 6; radius: 3
        color: theme.accent
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
