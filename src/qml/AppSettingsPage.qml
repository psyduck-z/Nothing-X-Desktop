import QtQuick

// Preferences for the desktop app itself (not the earbuds).
ScrollPage {
    objectName: "settings"
    title: qsTr("App settings")

    SectionLabel { text: qsTr("Appearance") }
    Card {
        Segmented {
            options: [
                { value: "system", label: qsTr("System") },
                { value: "dark", label: qsTr("Dark") },
                { value: "light", label: qsTr("Light") }
            ]
            current: settings.appearance
            onPicked: (v) => settings.appearance = v
        }
        ToggleRow {
            title: qsTr("Use %1 accent colour").arg(theme.systemName)
            subtitle: qsTr("Instead of Nothing red")
            checked: settings.systemAccent
            onToggled: (v) => settings.systemAccent = v
        }
    }

    SectionLabel { text: qsTr("Behaviour") }
    Card {
        ToggleRow {
            title: qsTr("Launch at login")
            subtitle: qsTr("Start quietly in the tray")
            checked: settings.launchAtLogin
            onToggled: (v) => settings.launchAtLogin = v
        }
        ToggleRow {
            title: qsTr("Connect automatically")
            subtitle: qsTr("When your earbuds connect to this computer")
            checked: settings.autoConnect
            onToggled: (v) => settings.autoConnect = v
        }
        ToggleRow {
            title: qsTr("Battery level in tray icon")
            subtitle: qsTr("The lower earbud, rounded to 5%")
            checked: settings.batteryInTray
            onToggled: (v) => settings.batteryInTray = v
        }
        ToggleRow {
            title: qsTr("Low battery notifications")
            subtitle: qsTr("At 20% and 10%")
            checked: settings.lowBatteryNotification
            onToggled: (v) => settings.lowBatteryNotification = v
        }
    }

    SectionLabel { text: qsTr("About") }
    Card {
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.text
            font.pixelSize: 13
            text: qsTr("Nothing X for desktop %1").arg(app.version)
        }
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.textMuted
            font.pixelSize: 11
            text: qsTr("An unofficial, open-source companion app. Not affiliated with Nothing Technology Ltd. The earbud protocol comes from community reverse-engineering work credited in the README.")
        }
        PillButton {
            text: qsTr("Quit Nothing X")
            icon: "power"
            primary: false
            onClicked: app.quit()
        }
    }
}
