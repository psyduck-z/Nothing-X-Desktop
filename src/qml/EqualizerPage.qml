import QtQuick

// Presets, the three-band custom EQ, and the eight-band advanced EQ.
ScrollPage {
    id: page
    title: qsTr("Equaliser")
    readonly property var v: device.values
    readonly property bool advanced: v.advancedEq === true
    readonly property int preset: v.eqPreset !== undefined ? v.eqPreset : -1
    readonly property var custom: v.customEq || [0, 0, 0]

    EqCurve {
        width: parent.width
        height: 96
        gains: page.advanced ? device.advancedBands.map(b => b.gain) : (page.preset === 5 ? [page.custom[0], page.custom[1], page.custom[2]] : [])
        preset: page.advanced ? -1 : page.preset
    }

    SectionLabel { text: qsTr("Presets") }
    Card {
        spacing: 0
        Repeater {
            model: [
                { value: 0, label: qsTr("Balanced"), sub: qsTr("Nothing's tuning") },
                { value: 3, label: qsTr("More bass"), sub: "" },
                { value: 2, label: qsTr("More treble"), sub: "" },
                { value: 1, label: qsTr("More voice"), sub: qsTr("Clearer vocals and podcasts") },
                { value: 5, label: qsTr("Custom"), sub: qsTr("Your own bass, mid and treble") }
            ]
            RadioRow {
                required property var modelData
                title: modelData.label
                subtitle: modelData.sub
                checked: !page.advanced && page.preset === modelData.value
                onClicked: device.setEqPreset(modelData.value)
            }
        }
        RadioRow {
            visible: device.features.includes("advancedEq")
            title: qsTr("Advanced")
            subtitle: qsTr("Eight-band parametric EQ")
            checked: page.advanced
            onClicked: device.setAdvancedEq(true)
        }
    }

    SectionLabel {
        visible: !page.advanced && page.preset === 5 && device.features.includes("customEq")
        text: qsTr("Custom")
    }
    Card {
        visible: !page.advanced && page.preset === 5 && device.features.includes("customEq")
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 44
            EqSlider { label: qsTr("Bass"); value: page.custom[0]; onMoved: (x) => device.setCustomEq(x, page.custom[1], page.custom[2]) }
            EqSlider { label: qsTr("Mid"); value: page.custom[1]; onMoved: (x) => device.setCustomEq(page.custom[0], x, page.custom[2]) }
            EqSlider { label: qsTr("Treble"); value: page.custom[2]; onMoved: (x) => device.setCustomEq(page.custom[0], page.custom[1], x) }
        }
        PillButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Reset")
            primary: false
            onClicked: device.setCustomEq(0, 0, 0)
        }
    }

    SectionLabel { visible: page.advanced; text: qsTr("Advanced") }
    Card {
        visible: page.advanced
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 3
            Repeater {
                model: device.advancedBands
                EqSlider {
                    required property var modelData
                    required property int index
                    width: 37
                    value: modelData.gain
                    label: modelData.frequency >= 1000 ? (modelData.frequency / 1000).toFixed(modelData.frequency % 1000 ? 1 : 0) + "k"
                                                        : Math.round(modelData.frequency)
                    onMoved: (x) => device.setAdvancedBandGain(index, x)
                }
            }
        }
        PillButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Reset to flat")
            primary: false
            onClicked: device.resetAdvancedEq()
        }
    }
}
