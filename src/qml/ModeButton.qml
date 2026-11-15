import QtQuick

// A round option with an official icon and a label: noise control and
// spatial audio modes on the home page.
Item {
    id: root
    property string icon
    property string label
    property bool selected: false
    property bool enabled2: true
    signal clicked()
    implicitWidth: 92
    implicitHeight: circle.height + 8 + text.implicitHeight
    opacity: enabled2 ? 1 : 0.4
    Rectangle {
        id: circle
        width: 56
        height: 56
        radius: 28
        anchors.horizontalCenter: parent.horizontalCenter
        color: root.selected ? theme.text : (area.containsMouse ? theme.outline : theme.surfaceRaised)
        Behavior on color { ColorAnimation { duration: 140 } }
        Icon {
            anchors.centerIn: parent
            path: root.icon
            size: 26
            color: root.selected ? theme.background : theme.text
        }
    }
    Txt {
        id: text
        anchors.top: circle.bottom
        anchors.topMargin: 8
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: root.label
        size: 12
        strong: root.selected
        color: root.selected ? theme.text : theme.textMuted
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled2
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
