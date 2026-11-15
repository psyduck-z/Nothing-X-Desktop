import QtQuick
import "Strings.js" as S

// The advanced equaliser's curve: log frequency 20 Hz-20 kHz across, -6 to
// +6 dB up, the summed response of the peaking bands, and a draggable point
// per band (drag sideways for frequency, up and down for gain).
Item {
    id: root
    property var bands: []
    property int selected: 0
    property bool editable: true
    signal select(int index)
    signal moved(int index, real frequency, real gain)
    implicitHeight: 210

    readonly property real plotLeft: 34
    readonly property real plotRight: width - 8
    readonly property real plotTop: 8
    readonly property real plotBottom: height - 22
    readonly property real maxDb: 6

    function xOf(f) { return plotLeft + (Math.log(f / 20) / Math.log(1000)) * (plotRight - plotLeft) }
    function fOf(x) { return 20 * Math.pow(1000, Math.max(0, Math.min(1, (x - plotLeft) / (plotRight - plotLeft)))) }
    function yOf(db) { return plotTop + (maxDb - db) / (2 * maxDb) * (plotBottom - plotTop) }
    function dbOf(y) { return maxDb - (y - plotTop) / (plotBottom - plotTop) * 2 * maxDb }

    // RBJ peaking filter magnitude at 48 kHz, in dB.
    function peakDb(f, f0, gain, q) {
        if (gain === 0) return 0
        const fs = 48000, A = Math.pow(10, gain / 40), w0 = 2 * Math.PI * f0 / fs, alpha = Math.sin(w0) / (2 * q)
        const b0 = 1 + alpha * A, b1 = -2 * Math.cos(w0), b2 = 1 - alpha * A
        const a0 = 1 + alpha / A, a1 = -2 * Math.cos(w0), a2 = 1 - alpha / A
        const w = 2 * Math.PI * f / fs
        const cr = Math.cos(w), ci = -Math.sin(w), c2r = Math.cos(2 * w), c2i = -Math.sin(2 * w)
        const nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i
        const dr = a0 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i
        return 10 * Math.log10((nr * nr + ni * ni) / (dr * dr + di * di))
    }

    onBandsChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    Connections { target: theme; function onChanged() { canvas.requestPaint() } }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.font = "10px '" + official.monoFont + "'"
            // Grid
            ctx.strokeStyle = theme.outline
            ctx.fillStyle = theme.textMuted
            ctx.lineWidth = 1
            for (const db of [-6, 0, 6]) {
                const y = Math.round(root.yOf(db)) + 0.5
                ctx.setLineDash(db === 0 ? [] : [2, 4])
                ctx.beginPath(); ctx.moveTo(root.plotLeft, y); ctx.lineTo(root.plotRight, y); ctx.stroke()
                ctx.fillText((db > 0 ? "+" : "") + db + "dB", 0, y + 3)
            }
            ctx.setLineDash([2, 4])
            const marks = [[20, ""], [100, "100Hz"], [1000, "1kHz"], [10000, "10kHz"], [20000, ""]]
            for (const [f, label] of marks) {
                const x = Math.round(root.xOf(f)) + 0.5
                ctx.beginPath(); ctx.moveTo(x, root.plotTop); ctx.lineTo(x, root.plotBottom); ctx.stroke()
                const w = ctx.measureText(label).width
                ctx.fillText(label, Math.min(root.plotRight - w, Math.max(root.plotLeft, x - w / 2)), root.height - 6)
            }
            ctx.setLineDash([])
            // Response
            ctx.strokeStyle = theme.text
            ctx.lineWidth = 2
            ctx.beginPath()
            for (let i = 0; i <= 160; ++i) {
                const f = 20 * Math.pow(1000, i / 160)
                let db = 0
                for (const b of root.bands)
                    db += root.peakDb(f, b.frequency, b.gain, b.q)
                const y = root.yOf(Math.max(-root.maxDb * 1.4, Math.min(root.maxDb * 1.4, db)))
                if (i === 0) ctx.moveTo(root.xOf(f), y); else ctx.lineTo(root.xOf(f), y)
            }
            ctx.stroke()
        }
    }

    Repeater {
        model: root.bands
        delegate: Rectangle {
            required property var modelData
            required property int index
            readonly property bool active: index === root.selected
            width: active ? 22 : 16
            height: width
            radius: width / 2
            x: root.xOf(modelData.frequency) - width / 2
            y: root.yOf(modelData.gain) - height / 2
            color: active ? theme.accent : theme.background
            border.width: 2
            border.color: active ? theme.accent : theme.text
            Txt {
                anchors.centerIn: parent
                text: index + 1
                size: 9
                strong: true
                color: active ? "#FFFFFF" : theme.text
            }
            MouseArea {
                anchors.fill: parent
                anchors.margins: -8
                cursorShape: root.editable ? Qt.SizeAllCursor : Qt.ArrowCursor
                preventStealing: true
                enabled: root.editable
                onPressed: root.select(index)
                onPositionChanged: (e) => {
                    if (!pressed) return
                    const p = mapToItem(root, e.x, e.y)
                    const db = Math.round(root.dbOf(p.y) * 2) / 2
                    root.moved(index, Math.round(root.fOf(p.x)), Math.max(-root.maxDb, Math.min(root.maxDb, db)))
                }
            }
        }
    }
}
