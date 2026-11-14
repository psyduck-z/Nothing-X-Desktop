import QtQuick

// A row that opens a sub-page, with an optional value on the right.
Item {
    id: root
    property string icon
    property string title
    property string value
    signal clicked()
    width: parent ? parent.width : 0
    implicitHeight: 44
    opacity: enabled ? 1 : 0.4

    Rectangle {
        anchors.fill: parent
        anchors.margins: -6
        radius: 12
        color: theme.text
        opacity: mouse.containsMouse ? 0.06 : 0
    }
    Icon {
        id: glyph
        name: root.icon
        visible: root.icon !== ""
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        anchors.left: glyph.visible ? glyph.right : parent.left
        anchors.leftMargin: glyph.visible ? 12 : 0
        anchors.right: valueText.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        color: theme.text
        font.pixelSize: 14
        elide: Text.ElideRight
    }
    Text {
        id: valueText
        anchors.right: chevron.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: root.value
        color: theme.textMuted
        font.pixelSize: 13
    }
    Icon {
        id: chevron
        name: "chevron"
        size: 16
        color: theme.textMuted
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
