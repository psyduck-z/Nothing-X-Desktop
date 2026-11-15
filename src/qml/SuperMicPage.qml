import QtQuick

// The official Super Mic page: the microphone in the case, walkie-talkie
// mode and automatic transparency during calls.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Super Mic")
    readonly property var v: device.values
    readonly property var f: device.features

    Card {
        padding: 0
        ProductImage {
            width: parent.width
            height: 150
            kind: "setting"
        }
    }
    Card {
        spacing: 6
        ListRow {
            visible: f.superMic === true
            icon: "assets/images/ear/super_mic.svg"
            title: qsTr("Super Mic")
            subtitle: qsTr("Use Super Mic for calls with a press and hold or double-press of the TALK button.")
            hasSwitch: true
            checked: v.superMic === true
            onToggled: (on) => device.setSuperMic(on)
        }
        ListRow {
            visible: f.walkieTalkie === true
            title: qsTr("Walkie-talkie mode")
            subtitle: qsTr("The microphones on your earbuds and charging case are muted by default. To speak during a call, press and hold or double-press the TALK button to activate Super Mic.")
            hasSwitch: true
            checked: v.walkieTalkie === true
            onToggled: (on) => {
                if (!on && v.superMic)
                    root.ui.confirm(qsTr("The walkie-talkie mode and Super Mic functions will be turned off simultaneously. Confirm disable?"),
                                    () => device.setWalkieTalkie(false))
                else
                    device.setWalkieTalkie(on)
            }
        }
        ListRow {
            visible: f.callTransparency === true
            icon: "assets/images/ear/auto_trans.svg"
            title: qsTr("Auto-transparency mode")
            subtitle: qsTr("Turn ON Transparency mode whenever you are on a phone call.")
            hasSwitch: true
            checked: v.callTransparency === true
            onToggled: (on) => device.setCallTransparency(on)
        }
    }
    Card {
        SectionLabel { text: qsTr("How to use") }
        Repeater {
            model: [
                qsTr("Press and hold the TALK button to use Super Mic. Release the button to return to the earbuds mic."),
                qsTr("Double-press the TALK button to use Super Mic for longer conversations. Return to the earbuds mic with a single press."),
                qsTr("Super Mic only works when the earbuds are in use and outside the case. Super Mic recommended distance from mouth: <40cm in normal noise environments and <5cm in loud noise environments, to ensure optimal call quality."),
                qsTr("With Dual Connection mode on, Super Mic will take longer to activate.")
            ]
            delegate: Row {
                required property string modelData
                width: parent.width
                spacing: 10
                Rectangle { width: 4; height: 4; radius: 2; y: 8; color: theme.accent }
                Txt { width: parent.width - 14; text: modelData; size: 13; color: theme.textMuted }
            }
        }
    }
}
