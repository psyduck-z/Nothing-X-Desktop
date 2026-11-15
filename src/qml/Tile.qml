import QtQuick

// A square-ish home tile: icon, title, the current state.
Rectangle {
    id: root
    property string icon
    property string title
    property string subtitle
    signal clicked()
    implicitHeight: 104
    radius: 20
    color: area.containsMouse ? theme.surfaceRaised : theme.surface
    Behavior on color { ColorAnimation { duration: 100 } }
    Icon { x: 16; y: 16; path: root.icon; size: 24 }
    Column {
        x: 16
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        width: parent.width - 32
        spacing: 2
        Txt { width: parent.width; text: root.title; size: 15; strong: true; wrapMode: Text.NoWrap }
        Txt { width: parent.width; text: root.subtitle; size: 12; color: theme.textMuted; wrapMode: Text.NoWrap; visible: text !== "" }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
