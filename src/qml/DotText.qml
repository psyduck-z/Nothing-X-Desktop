import QtQuick
import "DotGlyphs.js" as Glyphs

// Text drawn as a dot matrix, for the big numbers (battery levels) and the
// wordmark, the way Nothing's own UI does.
Row {
    id: root
    property string text: ""
    property real dot: 3
    property real gap: dot * 0.45
    property color color: "white"
    property color dimColor: "transparent"
    spacing: dot + gap

    Repeater {
        model: root.text.length
        Grid {
            required property int index
            readonly property var rows: Glyphs.glyph(root.text.charAt(index))
            columns: 5
            spacing: root.gap
            Repeater {
                model: 35
                Rectangle {
                    required property int index
                    readonly property bool on: parent.rows[Math.floor(index / 5)].charAt(index % 5) === "1"
                    width: root.dot
                    height: root.dot
                    radius: root.dot / 2
                    color: on ? root.color : root.dimColor
                }
            }
        }
    }
}
