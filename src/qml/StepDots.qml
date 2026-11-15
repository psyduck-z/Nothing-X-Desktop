import QtQuick

// A five-step slider drawn as dots, like Nothing's level pickers.
Item {
    id: root
    property int value: 1
    property int steps: 5
    signal picked(int value)
    width: parent ? parent.width : 0
    implicitHeight: 40
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        x: 10
        width: parent.width - 20
        height: 2
        color: theme.surfaceRaised
        Rectangle {
            height: parent.height
            width: parent.width * (root.value - 1) / (root.steps - 1)
            color: theme.text
        }
    }
    Repeater {
        model: root.steps
        delegate: Rectangle {
            required property int index
            readonly property bool active: index + 1 <= root.value
            width: index + 1 === root.value ? 20 : 10
            height: width
            radius: width / 2
            x: 10 + (root.width - 20) * index / (root.steps - 1) - width / 2
            anchors.verticalCenter: parent.verticalCenter
            color: active ? theme.text : theme.surfaceRaised
            border.width: index + 1 === root.value ? 4 : 0
            border.color: theme.surface
            Behavior on width { NumberAnimation { duration: 120 } }
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        function pick(mx) {
            const v = Math.round((mx - 10) / (root.width - 20) * (root.steps - 1)) + 1
            return Math.max(1, Math.min(root.steps, v))
        }
        onPressed: (e) => root.picked(pick(e.x))
        onPositionChanged: (e) => { if (pressed && pick(e.x) !== root.value) root.picked(pick(e.x)) }
    }
}
