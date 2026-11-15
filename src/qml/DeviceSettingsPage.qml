import QtQuick

// The official Device Settings page.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Device Settings")
    readonly property var v: device.values
    readonly property var f: device.features

    Card {
        spacing: 4
        ListRow {
            visible: f.inEar === true
            icon: "assets/images/ear/ear_in_ear_detection.svg"
            title: qsTr("In-ear detection")
            subtitle: qsTr("Automatically play audio when earbuds are in and pause when removed.")
            hasSwitch: true
            checked: v.inEar === true
            onToggled: (on) => device.setInEarDetection(on)
        }
        ListRow {
            visible: f.lowLag === true
            icon: "assets/images/ear/ear_low_lag_mode.svg"
            title: qsTr("Low-latency mode")
            subtitle: qsTr("Minimise latency for an improved gaming experience.")
            hasSwitch: true
            checked: v.lowLag === true
            onToggled: (on) => device.setLowLag(on)
        }
        ListRow {
            visible: f.ldac === true || f.lhdc === true
            icon: "assets/images/ear/ear_high_quality_audio.svg"
            title: qsTr("High-quality audio")
            value: v.ldac ? "LDAC" : "AAC"
            chevron: true
            onClicked: root.ui.open("hqa")
        }
        ListRow {
            visible: f.dual === true
            icon: "assets/images/ear/ear_dual_connection.svg"
            title: qsTr("Dual Connection")
            value: v.dual ? qsTr("On") : qsTr("Off")
            chevron: true
            onClicked: root.ui.open("dual")
        }
    }
    Card {
        spacing: 4
        ListRow {
            visible: f.superMic === true
            icon: "assets/images/ear/super_mic.svg"
            title: qsTr("Super Mic")
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
    }
    Card {
        spacing: 4
        ListRow {
            icon: "assets/images/device/device_firmware_update.svg"
            title: qsTr("Firmware")
            value: v.firmware || ""
            chevron: true
            onClicked: root.ui.open("about")
        }
        ListRow {
            icon: "packages/nt_widget/assets/images/info.svg"
            title: qsTr("Device details")
            chevron: true
            onClicked: root.ui.open("about")
        }
        ListRow {
            icon: "packages/nt_widget/assets/images/unbind.svg"
            title: qsTr("Disconnect")
            subtitle: qsTr("Stop controlling these earbuds from this computer.")
            chevron: true
            onClicked: device.disconnectFromDevice()
        }
    }
}
