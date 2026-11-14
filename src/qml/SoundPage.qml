import QtQuick

// Bass Enhance, spatial audio, personal sound and the codec.
ScrollPage {
    id: page
    title: qsTr("Sound")
    readonly property var v: device.values

    SectionLabel { visible: device.features.includes("bass"); text: qsTr("Bass Enhance") }
    Card {
        visible: device.features.includes("bass")
        ToggleRow {
            icon: "bass"
            title: qsTr("Bass Enhance")
            subtitle: qsTr("Deeper low end, tuned by Nothing")
            checked: v.bassEnhance === true
            onToggled: (on) => device.setBass(on, v.bassLevel || 3)
        }
        Segmented {
            enabled: v.bassEnhance === true
            options: [1, 2, 3, 4, 5].map(n => ({ value: n, label: String(n) }))
            current: v.bassLevel || 3
            onPicked: (n) => device.setBass(true, n)
        }
    }

    SectionLabel { visible: device.features.includes("spatial"); text: qsTr("Spatial audio") }
    Card {
        visible: device.features.includes("spatial")
        Segmented {
            options: [
                { value: 0, label: qsTr("Off") },
                { value: 1, label: qsTr("Fixed") }
            ]
            current: v.spatial || 0
            onPicked: (m) => device.setSpatial(m)
        }
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.textMuted
            font.pixelSize: 11
            text: qsTr("Fixed places the sound in front of you, like speakers in a room.")
        }
    }

    SectionLabel { visible: device.features.includes("personalSound") || device.features.includes("ldac"); text: qsTr("Quality") }
    Card {
        visible: device.features.includes("personalSound") || device.features.includes("ldac")
        ToggleRow {
            visible: device.features.includes("personalSound")
            icon: "hearing"
            title: qsTr("Personal sound")
            subtitle: qsTr("Uses the hearing profile you made in Nothing X on your phone")
            checked: v.personalSound === true
            onToggled: (on) => device.setPersonalSound(on)
        }
        ToggleRow {
            visible: device.features.includes("ldac")
            icon: "codec"
            title: qsTr("High quality audio (LDAC)")
            subtitle: qsTr("Hi-Res wireless audio. Uses more battery. The earbuds restart to switch.")
            checked: v.codec === 2
            onToggled: (on) => device.setCodec(on ? 2 : 0)
        }
        Text {
            visible: device.features.includes("ldac") && app.platform === "windows"
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.textMuted
            font.pixelSize: 11
            text: qsTr("Windows plays LDAC earbuds over AAC or SBC; LDAC still helps on phones that support it.")
        }
    }
}
