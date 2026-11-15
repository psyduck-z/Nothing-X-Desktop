import QtQuick
import QtQuick.Controls
import "Strings.js" as S

// The device page of the official app (EarMainPage): the earbuds and their
// battery, noise control, then every feature of the model as cards and rows.
Item {
    id: root
    property var ui
    signal back()

    readonly property bool connected: device.state === 2
    readonly property var f: device.features
    readonly property var v: device.values
    readonly property bool cancelling: device.ancMode >= 1 && device.ancMode <= 4

    // Top bar: the device name in NType 82, then devices and app settings.
    Item {
        id: top
        width: parent.width
        height: 52
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Headline {
                text: device.address !== "" || root.connected ? device.displayName : "Nothing X"
                size: 24
            }
            Row {
                spacing: 6
                Rectangle {
                    width: 6; height: 6; radius: 3
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.connected ? theme.good : device.state === 1 ? theme.charging : theme.textDim
                }
                SectionLabel {
                    text: root.connected ? qsTr("Connected")
                        : device.state === 1 ? qsTr("Connecting...")
                        : !scanner.adapterPoweredOn ? qsTr("Bluetooth is off")
                        : qsTr("Not connected")
                }
            }
        }
        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Repeater {
                model: [
                    { icon: "assets/images/device/device_ear_icon.svg", page: "devices", fallback: "◎" },
                    { icon: "packages/nt_widget/assets/images/settings.svg", page: "app", fallback: "⚙" }
                ]
                delegate: Rectangle {
                    required property var modelData
                    width: 36; height: 36; radius: 18
                    color: area.containsMouse ? theme.surfaceRaised : theme.surface
                    Icon { anchors.centerIn: parent; path: modelData.icon; size: 20 }
                    Txt { anchors.centerIn: parent; visible: !official.available; text: modelData.fallback; size: 16 }
                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.ui.open(modelData.page)
                    }
                }
            }
        }
    }

    Flickable {
        id: flick
        anchors.top: top.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: column.implicitHeight + 16
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { width: 4; policy: ScrollBar.AsNeeded }

        Column {
            id: column
            width: flick.width
            spacing: 12

            // --- The official look isn't imported yet ---------------------------
            Card {
                visible: !official.available
                SectionLabel { text: qsTr("Official look") }
                Txt {
                    width: parent.width
                    text: qsTr("Import the official Nothing X app (.xapk or .apk) to use Nothing's own fonts, icons and product images. It stays on this computer.")
                    size: 13
                    color: theme.textMuted
                }
                Pill {
                    text: official.importing ? qsTr("Importing...") : qsTr("Import Nothing X app")
                    primary: true
                    enabled2: !official.importing
                    onClicked: app.chooseOfficialApp()
                }
            }

            // --- Earbuds and battery ------------------------------------------------
            Card {
                padding: 16
                spacing: 8
                ProductImage {
                    width: parent.width
                    height: 168
                    kind: "global"
                    opacity: root.connected ? 1 : 0.35
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                    Dot {
                        anchors.centerIn: parent
                        visible: parent.status !== Image.Ready
                        text: root.connected ? device.displayName : "NOTHING X"
                        size: 38
                        color: theme.textMuted
                    }
                }
                BatteryView {
                    visible: root.connected
                    width: parent.width
                    showCase: device.batteryCase >= 0 || f.caseButton === true
                }
                Column {
                    visible: !root.connected
                    width: parent.width
                    spacing: 10
                    Txt {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: device.state === 1 ? qsTr("Connecting...")
                            : device.error !== "" ? device.error
                            : settings.lastDeviceAddress !== "" ? qsTr("Device disconnected")
                            : qsTr("Choose your earbuds to get started.")
                        size: 14
                        color: device.error !== "" ? theme.accent : theme.text
                    }
                    Txt {
                        width: parent.width
                        visible: device.state === 0 && settings.lastDeviceAddress !== "" && device.error === ""
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Take your earbuds out of the case, or connect them to this computer.")
                        size: 12
                        color: theme.textMuted
                    }
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 10
                        visible: device.state === 0
                        Pill {
                            visible: settings.lastDeviceAddress !== ""
                            text: qsTr("Reconnect")
                            primary: true
                            onClicked: device.connectToLast()
                        }
                        // With one pair of Nothing earbuds paired, one click is enough.
                        Pill {
                            readonly property var only: settings.lastDeviceAddress === "" && scanner.devices.length === 1 ? scanner.devices[0] : null
                            text: only ? qsTr("Use %1").arg(only.name) : settings.lastDeviceAddress !== "" ? qsTr("Other devices") : qsTr("Choose earbuds")
                            primary: settings.lastDeviceAddress === ""
                            onClicked: only ? device.connectToDevice(only.address, only.name) : root.ui.open("devices")
                        }
                    }
                }
            }

            // --- Noise control (EarAncCard) -------------------------------------------
            Card {
                visible: root.connected && f.anc === true
                SectionLabel { text: qsTr("Noise control") }
                Row {
                    width: parent.width
                    ModeButton {
                        width: parent.width / (f.transparency ? 3 : 2)
                        icon: "assets/images/ear/card_anc_on.svg"
                        label: qsTr("Noise Cancellation")
                        selected: root.cancelling
                        onClicked: device.setNoiseCancellation()
                    }
                    ModeButton {
                        visible: f.transparency === true
                        width: parent.width / 3
                        icon: "assets/images/ear/card_anc_transparency.svg"
                        label: qsTr("Transparency")
                        selected: device.ancMode === 7
                        onClicked: device.setAncMode(7)
                    }
                    ModeButton {
                        width: parent.width / (f.transparency ? 3 : 2)
                        icon: "assets/images/ear/card_anc_off.svg"
                        label: qsTr("Off")
                        selected: device.ancMode === 5
                        onClicked: device.setAncMode(5)
                    }
                }
                Segmented {
                    visible: root.cancelling && (f.ancMid || f.ancLow || f.ancAdaptive)
                    current: device.ancMode
                    model: {
                        const m = [{ label: qsTr("High"), value: 1 }]
                        if (f.ancMid) m.push({ label: qsTr("Mid"), value: 2 })
                        if (f.ancLow) m.push({ label: qsTr("Low"), value: 3 })
                        if (f.ancAdaptive) m.push({ label: qsTr("Adaptive"), value: 4 })
                        return m
                    }
                    onPicked: (value) => root.ui.guarded("anc", value, () => device.setAncMode(value))
                }
            }

            // --- Equaliser and Controls -----------------------------------------------
            Row {
                visible: root.connected && (f.eq === true || f.gestures === true)
                width: parent.width
                spacing: 12
                Tile {
                    visible: f.eq === true
                    width: f.gestures ? (parent.width - 12) / 2 : parent.width
                    icon: "assets/images/ear/card_equaliser.svg"
                    title: qsTr("Equaliser")
                    subtitle: v.advancedEq ? qsTr("Advanced") : S.presetName(v.eqPreset !== undefined ? v.eqPreset : 0)
                    onClicked: root.ui.open("eq")
                }
                Tile {
                    visible: f.gestures === true
                    width: f.eq ? (parent.width - 12) / 2 : parent.width
                    icon: "assets/images/ear/card_control.svg"
                    title: qsTr("Controls")
                    subtitle: qsTr("Gestures")
                    onClicked: root.ui.open("controls")
                }
            }

            // --- Spatial audio (EarSpatialAudioCard) ------------------------------------
            Card {
                visible: root.connected && f.spatial === true
                SectionLabel { text: qsTr("Spatial audio") }
                Row {
                    width: parent.width
                    Repeater {
                        model: {
                            const m = [
                                { label: qsTr("Off"), value: 0, icon: "assets/images/ear/ear_spatial_audio_off.svg" },
                                { label: qsTr("Fixed"), value: 1, icon: "assets/images/ear/ear_spatial_audio_fixed.svg" }
                            ]
                            if (f.spatialHead)
                                m.push({ label: qsTr("Head-tracking"), value: 2, icon: "assets/images/ear/ear_spatial_audio_tracked.svg" })
                            return m
                        }
                        delegate: ModeButton {
                            required property var modelData
                            width: parent.width / (f.spatialHead ? 3 : 2)
                            icon: modelData.icon
                            label: modelData.label
                            selected: (v.spatial || 0) === modelData.value
                            onClicked: root.ui.guarded("spatial", modelData.value, () => device.setSpatial(modelData.value))
                        }
                    }
                }
                Txt {
                    width: parent.width
                    text: qsTr("An immersive listening experience that makes you feel like you're part of the music, movie or game.")
                    size: 12
                    color: theme.textMuted
                }
            }

            // --- Bass Enhancement ----------------------------------------------------------
            Card {
                visible: root.connected && f.ultraBass === true
                ListRow {
                    title: qsTr("Bass Enhancement")
                    subtitle: v.bassOn ? qsTr("Level %1").arg(v.bassLevel || 1) : qsTr("Dynamically improves the impression of low frequencies in real-time.")
                    hasSwitch: true
                    checked: v.bassOn === true
                    onToggled: (on) => root.ui.guarded("bass", on, () => device.setBass(on, v.bassLevel || 1))
                }
                StepDots {
                    visible: v.bassOn === true
                    value: v.bassLevel || 1
                    onPicked: (level) => device.setBass(true, level)
                }
            }

            // --- Rows ---------------------------------------------------------------------------
            Card {
                visible: root.connected
                spacing: 4
                ListRow {
                    visible: f.personalSound === true
                    icon: "assets/images/ear/card_sound_personalisation.svg"
                    title: qsTr("Personal Sound Profile")
                    subtitle: qsTr("Set up your profile in the Nothing X phone app; switch it on or off here.")
                    hasSwitch: true
                    checked: v.personalSound === true
                    onToggled: (on) => root.ui.guarded("personalSound", on, () => device.setPersonalSound(on))
                }
                ListRow {
                    visible: f.superMic === true
                    icon: "assets/images/ear/super_mic.svg"
                    title: qsTr("Super Mic")
                    subtitle: qsTr("Manage Super Mic settings")
                    chevron: true
                    onClicked: root.ui.open("supermic")
                }
                ListRow {
                    visible: f.fitTest === true
                    icon: "assets/images/ear/ear_tip_fit_test.svg"
                    title: qsTr("Ear tip fit test")
                    chevron: true
                    onClicked: root.ui.open("fit")
                }
                ListRow {
                    visible: f.find === true
                    icon: "assets/images/device/device_ear_icon.svg"
                    title: qsTr("Find my earbuds")
                    chevron: true
                    onClicked: root.ui.open("find")
                }
                ListRow {
                    icon: "packages/nt_widget/assets/images/settings.svg"
                    title: qsTr("Device Settings")
                    chevron: true
                    onClicked: root.ui.open("settings")
                }
            }
        }
    }
}
