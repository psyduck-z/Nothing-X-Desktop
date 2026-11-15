import QtQuick
import "Strings.js" as S

// The official Equaliser page: Simple (presets and the 3-band custom EQ)
// and Advanced (the multi-band curve, with profiles kept on this computer).
ScrollPage {
    id: root
    property var ui
    property bool startAdvanced: false
    title: qsTr("Equaliser")

    readonly property var v: device.values
    readonly property int preset: v.eqPreset !== undefined ? v.eqPreset : 0
    property int tab: 0
    property int band: 0
    Component.onCompleted: tab = (startAdvanced || v.advancedEq === true) && device.features.advancedEq ? 1 : 0

    // Saved advanced profiles, per earbuds.
    property var profiles: settings.deviceValue(device.address, "eqProfiles", [])
    function saveProfiles(list) {
        settings.setDeviceValue(device.address, "eqProfiles", list)
        profiles = list
    }

    Txt {
        width: parent.width
        text: qsTr("Customise your sound or select a pre-tuned preset.")
        size: 13
        color: theme.textMuted
    }

    Segmented {
        visible: device.features.advancedEq === true
        current: root.tab
        model: [{ label: qsTr("Simple"), value: 0 }, { label: qsTr("Advanced"), value: 1 }]
        onPicked: (value) => {
            if (value === 1)
                root.ui.guarded("advancedEq", true, () => { root.tab = 1; device.setAdvancedEq(true) })
            else {
                root.tab = 0
                if (v.advancedEq)
                    device.setAdvancedEq(false)
            }
        }
    }

    // --- Simple ------------------------------------------------------------------
    Repeater {
        model: root.tab === 0 ? device.eqPresets : []
        delegate: Rectangle {
            required property var modelData
            readonly property bool selected: !v.advancedEq && root.preset === modelData
            width: parent.width
            implicitHeight: content.implicitHeight + 28
            radius: 20
            color: selected ? theme.surfaceRaised : theme.surface
            border.width: selected ? 2 : 0
            border.color: theme.text
            Column {
                id: content
                x: 16; y: 14
                width: parent.width - 32
                spacing: 14
                Row {
                    width: parent.width
                    spacing: 14
                    Icon { path: S.presetIcon(modelData); size: 28; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        width: parent.width - 80
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        Txt { width: parent.width; text: qsTr(S.presetName(modelData)); size: 15; strong: true }
                        Txt { width: parent.width; text: qsTr(S.presetDescription(modelData)); size: 12; color: theme.textMuted }
                    }
                    Rectangle {
                        width: 22; height: 22; radius: 11
                        anchors.verticalCenter: parent.verticalCenter
                        color: selected ? theme.text : "transparent"
                        border.width: 2
                        border.color: selected ? theme.text : theme.outline
                        Rectangle { anchors.centerIn: parent; width: 8; height: 8; radius: 4; color: theme.surfaceRaised; visible: selected }
                    }
                }
                // The custom equaliser: bass, mid, treble.
                Row {
                    visible: modelData === 5 && selected
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 18
                    EqBandSlider {
                        label: qsTr("Bass")
                        value: v.customBass || 0
                        onChanged2: (g) => device.setCustomEq(g, v.customMid || 0, v.customTreble || 0)
                    }
                    EqBandSlider {
                        label: qsTr("Mid")
                        value: v.customMid || 0
                        onChanged2: (g) => device.setCustomEq(v.customBass || 0, g, v.customTreble || 0)
                    }
                    EqBandSlider {
                        label: qsTr("Treble")
                        value: v.customTreble || 0
                        onChanged2: (g) => device.setCustomEq(v.customBass || 0, v.customMid || 0, g)
                    }
                }
            }
            MouseArea {
                anchors.fill: parent
                z: -1
                cursorShape: Qt.PointingHandCursor
                onClicked: root.ui.guarded("eqPreset", modelData, () => device.setEqPreset(modelData))
            }
        }
    }

    // --- Advanced -----------------------------------------------------------------
    Card {
        visible: root.tab === 1
        EqCurve {
            width: parent.width
            bands: device.advancedBands
            selected: root.band
            onSelect: (i) => root.band = i
            onMoved: (i, f, g) => {
                const b = device.advancedBands[i]
                device.setAdvancedBand(i, f, g, b.q)
            }
        }
        Row {
            width: parent.width
            spacing: 6
            Repeater {
                model: device.advancedBands
                delegate: Rectangle {
                    required property int index
                    width: 28; height: 28; radius: 14
                    color: index === root.band ? theme.text : theme.surfaceRaised
                    Txt { anchors.centerIn: parent; text: index + 1; size: 12; strong: true; color: index === root.band ? theme.background : theme.text }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.band = index }
                }
            }
            Item { width: 1; height: 1 }
            Rectangle {
                width: 28; height: 28; radius: 14
                visible: device.advancedBands.length < 8
                color: addArea.containsMouse ? theme.outline : theme.surfaceRaised
                Icon { anchors.centerIn: parent; path: "packages/nt_widget/assets/images/add.svg"; size: 16 }
                Txt { anchors.centerIn: parent; visible: !official.available; text: "+"; size: 16 }
                MouseArea { id: addArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: device.addAdvancedBand() }
            }
        }
        Item {
            width: parent.width
            height: 64
            readonly property var b: device.advancedBands[Math.min(root.band, device.advancedBands.length - 1)] || { frequency: 1000, gain: 0, q: 1 }
            readonly property int i: Math.min(root.band, device.advancedBands.length - 1)
            Row {
                anchors.fill: parent
                spacing: 8
                Stepper {
                    width: (parent.width - 16) / 3
                    label: qsTr("Frequency")
                    text: S.frequencyLabel(parent.parent.b.frequency)
                    onMinus: device.setAdvancedBand(parent.parent.i, parent.parent.b.frequency / 1.06, parent.parent.b.gain, parent.parent.b.q)
                    onPlus: device.setAdvancedBand(parent.parent.i, parent.parent.b.frequency * 1.06, parent.parent.b.gain, parent.parent.b.q)
                }
                Stepper {
                    width: (parent.width - 16) / 3
                    label: qsTr("Gain")
                    text: (parent.parent.b.gain > 0 ? "+" : "") + parent.parent.b.gain.toFixed(1) + " dB"
                    onMinus: device.setAdvancedBand(parent.parent.i, parent.parent.b.frequency, parent.parent.b.gain - 0.5, parent.parent.b.q)
                    onPlus: device.setAdvancedBand(parent.parent.i, parent.parent.b.frequency, parent.parent.b.gain + 0.5, parent.parent.b.q)
                }
                Stepper {
                    width: (parent.width - 16) / 3
                    label: qsTr("Q factor")
                    text: parent.parent.b.q.toFixed(1)
                    onMinus: device.setAdvancedBand(parent.parent.i, parent.parent.b.frequency, parent.parent.b.gain, parent.parent.b.q - 0.1)
                    onPlus: device.setAdvancedBand(parent.parent.i, parent.parent.b.frequency, parent.parent.b.gain, parent.parent.b.q + 0.1)
                }
            }
        }
        Txt {
            width: parent.width
            text: qsTr("Frequency controls where to adjust, in Hz. Q factor controls how wide the adjustment affects, from narrow to broad. Gain controls how much to boost or cut, in dB.")
            size: 12
            color: theme.textMuted
        }
        Row {
            width: parent.width
            spacing: 8
            SectionLabel {
                width: parent.width - 196
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Total gain") + " " + device.features.advancedTotalGain + " dB"
            }
            Pill {
                text: qsTr("Remove")
                enabled2: device.advancedBands.length > 1
                implicitWidth: 90
                onClicked: { device.removeAdvancedBand(root.band); root.band = Math.max(0, root.band - 1) }
            }
            Pill {
                text: qsTr("Reset")
                implicitWidth: 90
                onClicked: root.ui.confirm(qsTr("This action will reset all changes to this profile"), () => device.resetAdvancedEq(), qsTr("Reset profile?"))
            }
        }
    }

    // Profiles, kept on this computer like the official app keeps them on the phone.
    Card {
        visible: root.tab === 1
        spacing: 4
        SectionLabel { text: qsTr("Profiles") }
        Repeater {
            model: root.profiles
            delegate: ListRow {
                required property var modelData
                required property int index
                title: modelData.name
                subtitle: modelData.bands.length + " " + qsTr("bands")
                chevron: true
                onClicked: { device.setAdvancedBands(modelData.bands); root.ui.toast(qsTr("Profile applied")) }
                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: 28
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28; height: 28; radius: 14
                    color: delArea.containsMouse ? theme.surfaceRaised : "transparent"
                    Icon { anchors.centerIn: parent; path: "packages/nt_widget/assets/images/delete.svg"; size: 16; color: theme.textMuted }
                    Txt { anchors.centerIn: parent; visible: !official.available; text: "×"; size: 16 }
                    MouseArea {
                        id: delArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.ui.confirm(qsTr("Are you sure you want to delete this profile?"), () => {
                            const list = root.profiles.slice()
                            list.splice(index, 1)
                            root.saveProfiles(list)
                        }, qsTr("Delete profile"))
                    }
                }
            }
        }
        Txt {
            visible: root.profiles.length === 0
            width: parent.width
            text: qsTr("Save the current curve to come back to it later.")
            size: 12
            color: theme.textMuted
        }
        Pill {
            text: qsTr("New profile")
            enabled2: root.profiles.length < 20
            onClicked: {
                const list = root.profiles.slice()
                list.push({ name: qsTr("Profile %1").arg(list.length + 1), bands: device.advancedBands })
                root.saveProfiles(list)
                root.ui.toast(qsTr("New profile added"))
            }
        }
    }
}
