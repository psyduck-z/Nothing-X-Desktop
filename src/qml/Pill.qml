import QtQuick

// A pill button: filled for the main action, outlined otherwise.
Rectangle {
    id: root
    property string text
    property bool primary: false
    property bool danger: false
    property bool enabled2: true
    signal clicked()
    implicitWidth: Math.max(96, label.implicitWidth + 36)
    implicitHeight: 42
    radius: height / 2
    opacity: enabled2 ? 1 : 0.4
    color: primary ? (danger ? theme.accent : theme.text) : (area.containsMouse ? theme.surfaceRaised : "transparent")
    border.width: primary ? 0 : 1
    border.color: theme.outline
    Txt {
        id: label
        anchors.centerIn: parent
        text: root.text
        size: 14
        strong: true
        wrapMode: Text.NoWrap
        color: root.primary ? (root.danger ? "#FFFFFF" : theme.background) : theme.text
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
