import QtQuick

// A vertical gain slider from -6 to +6 dB in 0.5 dB steps, drawn as a thin
// track with a dot knob. Emits moved() while dragging.
Item {
    id: root
    property real value: 0
    property real from: -6
    property real to: 6
    property real step: 0.5
    property string label
    signal moved(real value)
    width: 36
    height: 190

    readonly property real trackTop: 22
    readonly property real trackHeight: height - trackTop - 24
    readonly property real ratio: (value - from) / (to - from)

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        text: (root.value > 0 ? "+" : "") + root.value.toFixed(root.step < 1 && root.value % 1 !== 0 ? 1 : 0)
        color: theme.text
        font.pixelSize: 11
    }
    Rectangle {
        id: track
        x: (root.width - width) / 2
        y: root.trackTop
        width: 4
        height: root.trackHeight
        radius: 2
        color: theme.surfaceRaised
        // Fill from 0 dB to the value.
        Rectangle {
            readonly property real zeroY: parent.height * (1 - (0 - root.from) / (root.to - root.from))
            readonly property real valueY: parent.height * (1 - root.ratio)
            width: parent.width
            radius: 2
            y: Math.min(zeroY, valueY)
            height: Math.abs(zeroY - valueY)
            color: theme.accent
        }
    }
    Rectangle {
        width: 18
        height: 18
        radius: 9
        x: (root.width - width) / 2
        y: root.trackTop + root.trackHeight * (1 - root.ratio) - height / 2
        color: theme.inverse
        border.width: 3
        border.color: theme.surface
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: root.label
        color: theme.textMuted
        font.pixelSize: 10
    }
    MouseArea {
        anchors.fill: parent
        preventStealing: true
        cursorShape: Qt.PointingHandCursor
        function update(mouseY) {
            const r = 1 - Math.max(0, Math.min(1, (mouseY - root.trackTop) / root.trackHeight))
            const v = Math.round((root.from + r * (root.to - root.from)) / root.step) * root.step
            if (v !== root.value)
                root.moved(v)
        }
        onPressed: (mouse) => update(mouse.y)
        onPositionChanged: (mouse) => update(mouse.y)
        onWheel: (wheel) => root.moved(Math.max(root.from, Math.min(root.to, root.value + (wheel.angleDelta.y > 0 ? root.step : -root.step))))
    }
}
