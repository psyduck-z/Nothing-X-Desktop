import QtQuick

// A row of mutually exclusive choices: [{value, label, icon?}].
Rectangle {
    id: root
    property var options: []
    property var current
    signal picked(var value)
    width: parent ? parent.width : 0
    implicitHeight: 38
    radius: height / 2
    color: theme.surfaceRaised
    opacity: enabled ? 1 : 0.4

    Row {
        anchors.fill: parent
        anchors.margins: 3
        Repeater {
            model: root.options
            Rectangle {
                required property var modelData
                readonly property bool active: modelData.value === root.current
                width: (root.width - 6) / Math.max(1, root.options.length)
                height: root.height - 6
                radius: height / 2
                color: active ? theme.inverse : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
                Text {
                    anchors.centerIn: parent
                    width: parent.width - 8
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData.label
                    color: parent.active ? theme.onInverse : theme.text
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: root.enabled
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(modelData.value)
                }
            }
        }
    }
}
