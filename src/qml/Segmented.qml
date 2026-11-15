import QtQuick

// A row of options, the chosen one filled. model: [{label, value}].
Rectangle {
    id: root
    property var model: []
    property var current
    property int itemHeight: 36
    signal picked(var value)
    implicitHeight: itemHeight + 8
    width: parent ? parent.width : 0
    radius: implicitHeight / 2
    color: theme.surfaceRaised
    Row {
        anchors.fill: parent
        anchors.margins: 4
        Repeater {
            model: root.model
            delegate: Rectangle {
                required property var modelData
                readonly property bool selected: modelData.value === root.current
                width: (root.width - 8) / Math.max(1, root.model.length)
                height: root.itemHeight
                radius: height / 2
                color: selected ? theme.text : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
                Txt {
                    anchors.centerIn: parent
                    width: parent.width - 8
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData.label
                    size: 13
                    strong: selected
                    wrapMode: Text.NoWrap
                    color: selected ? theme.background : theme.text
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(modelData.value)
                }
            }
        }
    }
}
