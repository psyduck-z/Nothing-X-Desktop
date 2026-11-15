import QtQuick

// A short message at the bottom of the popup.
Rectangle {
    id: root
    property alias text: label.text
    width: Math.min(parent.width - 32, label.implicitWidth + 32)
    height: label.implicitHeight + 20
    radius: height / 2
    color: theme.text
    opacity: 0
    visible: opacity > 0
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 20
    Txt {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, root.parent.width - 64)
        horizontalAlignment: Text.AlignHCenter
        size: 13
        color: theme.background
    }
    function show(message) {
        label.text = message
        fade.stop()
        opacity = 1
        hold.restart()
    }
    Timer { id: hold; interval: 3200; onTriggered: fade.start() }
    NumberAnimation { id: fade; target: root; property: "opacity"; to: 0; duration: 300 }
}
