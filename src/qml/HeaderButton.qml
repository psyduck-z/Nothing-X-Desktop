import QtQuick
import QtQuick.Controls

// A small circular icon button for the header.
Rectangle {
    id: root
    property string icon
    property string tip
    signal clicked()
    width: 36
    height: 36
    radius: 18
    color: mouse.containsMouse ? theme.surfaceRaised : theme.surface
    Icon {
        anchors.centerIn: parent
        name: root.icon
        size: 18
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    ToolTip.visible: tip !== "" && mouse.containsMouse
    ToolTip.text: tip
    ToolTip.delay: 600
}
