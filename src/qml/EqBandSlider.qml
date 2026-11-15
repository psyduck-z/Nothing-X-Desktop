import QtQuick

// One vertical band of the custom equaliser: -6 to +6 dB in 0.5 dB steps.
Item {
    id: root
    property string label
    property real value: 0
    property real minimum: -6
    property real maximum: 6
    signal changed2(real value)
    implicitWidth: 72
    implicitHeight: 200
    readonly property real trackTop: 28
    readonly property real trackBottom: height - 30
    function yOf(v) { return trackTop + (maximum - v) / (maximum - minimum) * (trackBottom - trackTop) }

    Dot {
        anchors.horizontalCenter: parent.horizontalCenter
        text: (root.value > 0 ? "+" : "") + root.value
        size: 18
        color: root.value !== 0 ? theme.text : theme.textMuted
    }
    Rectangle {
        id: track
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.trackTop
        width: 6
        height: root.trackBottom - root.trackTop
        radius: 3
        color: theme.surfaceRaised
        Rectangle {
            x: 0
            width: parent.width
            radius: 3
            color: theme.text
            y: Math.min(root.yOf(root.value), root.yOf(0)) - root.trackTop
            height: Math.abs(root.yOf(root.value) - root.yOf(0))
        }
    }
    Rectangle {
        width: 24; height: 24; radius: 12
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.yOf(root.value) - 12
        color: theme.text
        border.width: 4
        border.color: theme.surface
    }
    Txt {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        size: 13
        color: theme.textMuted
    }
    MouseArea {
        x: 0
        y: root.trackTop - 12
        width: parent.width
        height: root.trackBottom - root.trackTop + 24
        cursorShape: Qt.PointingHandCursor
        preventStealing: true
        function pick(my) {
            const v = root.maximum - (my + y - root.trackTop) / (root.trackBottom - root.trackTop) * (root.maximum - root.minimum)
            return Math.max(root.minimum, Math.min(root.maximum, Math.round(v * 2) / 2))
        }
        onPressed: (e) => root.changed2(pick(e.y))
        onPositionChanged: (e) => { if (pressed) root.changed2(pick(e.y)) }
        onWheel: (w) => root.changed2(Math.max(root.minimum, Math.min(root.maximum, root.value + (w.angleDelta.y > 0 ? 0.5 : -0.5))))
    }
}
