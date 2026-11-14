import QtQuick

// One option in a list where exactly one is chosen.
Item {
    id: root
    property string title
    property string subtitle
    property bool checked
    signal clicked()
    width: parent ? parent.width : 0
    implicitHeight: Math.max(42, texts.implicitHeight + 8)

    Column {
        id: texts
        anchors.left: parent.left
        anchors.right: dot.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Text { width: parent.width; text: root.title; color: theme.text; font.pixelSize: 14 }
        Text { width: parent.width; visible: text !== ""; text: root.subtitle; color: theme.textMuted; font.pixelSize: 11; wrapMode: Text.WordWrap }
    }
    Rectangle {
        id: dot
        width: 20; height: 20; radius: 10
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: "transparent"
        border.width: 1.5
        border.color: root.checked ? theme.accent : theme.textMuted
        Rectangle {
            anchors.centerIn: parent
            width: 10; height: 10; radius: 5
            visible: root.checked
            color: theme.accent
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
