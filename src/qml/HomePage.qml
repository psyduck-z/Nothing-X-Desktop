import QtQuick
import QtQuick.Controls

// The popup's main page: device, batteries, noise control, and the way into
// every other setting.
Item {
    id: page
    signal open(string page)
    readonly property bool connected: device.state === 2
    readonly property var v: device.values

    function eqName(preset) {
        if (v.advancedEq) return qsTr("Advanced")
        switch (preset) {
        case 0: return qsTr("Balanced")
        case 1: return qsTr("More voice")
        case 2: return qsTr("More treble")
        case 3: return qsTr("More bass")
        case 5: return qsTr("Custom")
        }
        return ""
    }

    // --- Header ---------------------------------------------------------------
    Item {
        id: header
        width: parent.width
        height: 52

        Column {
            anchors.left: parent.left
            anchors.right: headerButtons.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            Text {
                width: parent.width
                text: page.connected || device.state === 1 ? device.displayName : "Nothing X"
                color: theme.text
                font.pixelSize: 20
                font.weight: Font.Medium
                elide: Text.ElideRight
            }
            Row {
                spacing: 6
                Rectangle {
                    width: 7; height: 7; radius: 3.5
                    anchors.verticalCenter: parent.verticalCenter
                    color: page.connected ? theme.accent : theme.textMuted
                    SequentialAnimation on opacity {
                        running: device.state === 1
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.2; duration: 500 }
                        NumberAnimation { to: 1; duration: 500 }
                    }
                }
                Text {
                    color: theme.textMuted
                    font.pixelSize: 12
                    text: page.connected ? (app.demo ? qsTr("Connected · demo") : qsTr("Connected"))
                        : device.state === 1 ? qsTr("Connecting…") : qsTr("Not connected")
                }
            }
        }
        Row {
            id: headerButtons
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            HeaderButton { icon: "devices"; onClicked: page.open("devices"); tip: qsTr("Devices") }
            HeaderButton { icon: "settings"; onClicked: page.open("settings"); tip: qsTr("App settings") }
        }
    }

    // --- Body -----------------------------------------------------------------
    Flickable {
        anchors.top: header.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: body.implicitHeight + 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 4 }

        Column {
            id: body
            width: parent.width
            spacing: 12

            // Not connected ---------------------------------------------------
            Card {
                visible: !page.connected
                padding: 20
                spacing: 14
                DotText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: device.state === 1 ? "..." : "NO EAR"
                    dot: 4
                    color: theme.text
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: theme.textMuted
                    font.pixelSize: 13
                    text: device.state === 1
                        ? qsTr("Connecting to %1…").arg(device.bluetoothName)
                        : device.error !== ""
                            ? qsTr("Couldn't connect: %1. Make sure the earbuds are out of the case and connected to this computer.").arg(device.error)
                            : settings.lastDeviceAddress !== ""
                                ? qsTr("Take your earbuds out of the case, then connect.")
                                : qsTr("Pair your Nothing earbuds with this computer, then choose them here.")
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10
                    PillButton {
                        visible: settings.lastDeviceAddress !== "" && device.state === 0
                        text: qsTr("Connect")
                        icon: "link"
                        onClicked: device.connectToLast()
                    }
                    PillButton {
                        text: qsTr("Choose device")
                        primary: settings.lastDeviceAddress === ""
                        icon: "devices"
                        onClicked: page.open("devices")
                    }
                }
            }

            // Batteries ---------------------------------------------------------
            Card {
                visible: page.connected
                padding: 18
                Row {
                    width: parent.width
                    readonly property int columns: device.hasCase ? 3 : 2
                    BatteryView {
                        width: parent.width / parent.columns
                        label: qsTr("Left")
                        icon: "budleft"
                        level: device.batteryLeft
                        charging: device.chargingLeft
                        inEar: v.leftInEar === true
                    }
                    BatteryView {
                        width: parent.width / parent.columns
                        label: qsTr("Right")
                        icon: "budright"
                        level: device.batteryRight
                        charging: device.chargingRight
                        inEar: v.rightInEar === true
                    }
                    BatteryView {
                        visible: device.hasCase
                        width: parent.width / parent.columns
                        label: qsTr("Case")
                        icon: "case"
                        level: device.batteryCase
                        charging: device.chargingCase
                    }
                }
                Text {
                    visible: device.hasCase && device.batteryCase < 0
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    color: theme.textMuted
                    font.pixelSize: 11
                    text: qsTr("Open the case lid to see its battery")
                }
            }

            // Noise control -----------------------------------------------------
            SectionLabel {
                visible: page.connected && device.features.includes("anc")
                text: qsTr("Noise control")
            }
            Card {
                visible: page.connected && device.features.includes("anc")
                spacing: 12
                Row {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: [
                            { mode: 3, label: qsTr("Noise\ncancellation"), icon: "anc" },
                            { mode: 1, label: qsTr("Transparency"), icon: "transparency" },
                            { mode: 0, label: qsTr("Off"), icon: "off" }
                        ]
                        ModeTile {
                            required property var modelData
                            width: (parent.width - 16) / 3
                            icon: modelData.icon
                            label: modelData.label
                            active: modelData.mode === 3 ? device.ancMode >= 2 : device.ancMode === modelData.mode
                            onClicked: device.setAncMode(modelData.mode === 3 ? device.lastNoiseCancellingMode() : modelData.mode)
                        }
                    }
                }
                Segmented {
                    visible: device.ancMode >= 2 && (device.ancLevels || device.ancAdaptive)
                    options: {
                        let o = []
                        if (device.ancAdaptive) o.push({ value: 2, label: qsTr("Adaptive") })
                        if (device.ancLevels) {
                            o.push({ value: 3, label: qsTr("High") })
                            o.push({ value: 4, label: qsTr("Mid") })
                            o.push({ value: 5, label: qsTr("Low") })
                        }
                        return o
                    }
                    current: device.ancMode
                    onPicked: (value) => device.setAncMode(value)
                }
            }

            // Sound and controls -------------------------------------------------
            SectionLabel { visible: page.connected; text: qsTr("Sound & controls") }
            Card {
                visible: page.connected
                spacing: 6
                NavRow {
                    visible: device.features.includes("eq")
                    icon: "eq"
                    title: qsTr("Equaliser")
                    value: page.eqName(v.eqPreset)
                    onClicked: page.open("eq")
                }
                NavRow {
                    visible: device.features.includes("bass") || device.features.includes("spatial") || device.features.includes("ldac") || device.features.includes("personalSound")
                    icon: "bass"
                    title: qsTr("Sound")
                    value: v.bassEnhance ? qsTr("Bass %1").arg(v.bassLevel) : ""
                    onClicked: page.open("sound")
                }
                NavRow {
                    visible: device.features.includes("gestures")
                    icon: "gesture"
                    title: qsTr("Controls")
                    onClicked: page.open("gestures")
                }
                NavRow {
                    visible: device.features.includes("superMic") || device.features.includes("callTransparency")
                    icon: "mic"
                    title: qsTr("Microphone & calls")
                    value: v.superMic ? qsTr("Super Mic") : ""
                    onClicked: page.open("mic")
                }
            }

            // Tools ------------------------------------------------------------
            SectionLabel { visible: page.connected; text: qsTr("Tools") }
            Card {
                visible: page.connected
                spacing: 6
                NavRow {
                    visible: device.features.includes("fitTest")
                    icon: "eartip"
                    title: qsTr("Ear tip fit test")
                    onClicked: page.open("fit")
                }
                NavRow {
                    visible: device.features.includes("findBuds")
                    icon: "find"
                    title: qsTr("Find my earbuds")
                    onClicked: page.open("find")
                }
                NavRow {
                    visible: device.features.includes("dualConnection")
                    icon: "devices"
                    title: qsTr("Dual connection")
                    value: v.dualConnection === true ? qsTr("On") : v.dualConnection === false ? qsTr("Off") : ""
                    onClicked: page.open("dual")
                }
            }

            // Quick settings ----------------------------------------------------
            SectionLabel { visible: page.connected; text: qsTr("Settings") }
            Card {
                visible: page.connected
                spacing: 4
                ToggleRow {
                    visible: device.features.includes("inEar")
                    icon: "inear"
                    title: qsTr("In-ear detection")
                    subtitle: qsTr("Pause when you take an earbud out")
                    checked: v.inEarDetection === true
                    onToggled: (on) => device.setInEarDetection(on)
                }
                ToggleRow {
                    visible: device.features.includes("lowLatency")
                    icon: "latency"
                    title: qsTr("Low lag mode")
                    subtitle: qsTr("Less audio delay for games and video")
                    checked: v.lowLatency === true
                    onToggled: (on) => device.setLowLatency(on)
                }
                NavRow {
                    icon: "info"
                    title: qsTr("Device information")
                    value: v.firmware || ""
                    onClicked: page.open("about")
                }
            }

            Item { width: 1; height: 4 }
        }
    }
}
