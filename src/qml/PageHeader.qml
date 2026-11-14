import QtQuick

// The title bar of a sub-page, with a back arrow.
Item {
    id: root
    property string title
    signal back()
    width: parent ? parent.width : 0
    height: 44

    Rectangle {
        id: backButton
        width: 34
        height: 34
        radius: 17
        anchors.verticalCenter: parent.verticalCenter
        color: theme.surfaceRaised
        Icon {
            anchors.centerIn: parent
            name: "back"
            size: 18
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.back()
        }
    }
    Text {
        anchors.left: backButton.right
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        color: theme.text
        font.pixelSize: 18
        font.weight: Font.Medium
        elide: Text.ElideRight
    }
}
