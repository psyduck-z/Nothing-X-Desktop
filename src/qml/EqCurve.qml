import QtQuick

// A dot-matrix picture of the EQ: one column of dots per band, lit up to the
// band's gain. Presets show a representative shape.
Item {
    id: root
    property var gains: []
    property int preset: -1

    readonly property var shape: {
        if (gains.length > 0) return gains
        switch (preset) {
        case 3: return [5, 4, 2, 0, 0, 0, 0, 0]
        case 2: return [0, 0, 0, 0, 1, 3, 4, 5]
        case 1: return [-2, -1, 0, 2, 4, 3, 0, -1]
        default: return [0, 0, 0, 0, 0, 0, 0, 0]
        }
    }
    readonly property int columns: 24
    readonly property int rows: 9

    // Linear interpolation of the bands across the dot columns.
    function gainAt(c) {
        const n = shape.length
        if (n === 1) return shape[0]
        const pos = c / (columns - 1) * (n - 1)
        const i = Math.floor(pos), f = pos - i
        return i >= n - 1 ? shape[n - 1] : shape[i] * (1 - f) + shape[i + 1] * f
    }

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: theme.surface
    }
    Grid {
        anchors.centerIn: parent
        columns: root.columns
        rows: root.rows
        columnSpacing: (root.width - 40 - root.columns * 5) / (root.columns - 1)
        rowSpacing: (root.height - 24 - root.rows * 5) / (root.rows - 1)
        flow: Grid.TopToBottom
        Repeater {
            model: root.columns * root.rows
            Rectangle {
                required property int index
                readonly property int col: Math.floor(index / root.rows)
                readonly property int row: index % root.rows
                // Row 4 is 0 dB; each row is 1.5 dB.
                readonly property real level: (4 - row) * 1.5
                readonly property real g: root.gainAt(col)
                readonly property bool lit: row === 4 || (g > 0 ? level > 0 && level <= g + 0.01 : level < 0 && level >= g - 0.01)
                width: 5; height: 5; radius: 2.5
                color: lit ? (row === 4 ? theme.textMuted : theme.text) : theme.surfaceRaised
                Behavior on color { ColorAnimation { duration: 90 } }
            }
        }
    }
}
