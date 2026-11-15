import QtQuick

// A sub-page's top bar: back arrow and the title in NType 82.
Item {
    id: root
    property string title
    signal back()
    width: parent ? parent.width : 0
    implicitHeight: 52
    Rectangle {
        id: backButton
        width: 36
        height: 36
        radius: 18
        anchors.verticalCenter: parent.verticalCenter
        color: backArea.containsMouse ? theme.surfaceRaised : theme.surface
        Icon {
            anchors.centerIn: parent
            path: "packages/nt_widget/assets/images/arrow_back.svg"
            size: 20
        }
        Txt { anchors.centerIn: parent; visible: !official.available; text: "‹"; size: 22 }
        MouseArea {
            id: backArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.back()
        }
    }
    Headline {
        anchors.left: backButton.right
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        size: 22
    }
}
