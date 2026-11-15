import QtQuick

// This app's own settings: start-up, tray, look, the imported official app
// and updates.
ScrollPage {
    id: root
    property var ui
    title: qsTr("App settings")

    Card {
        spacing: 4
        SectionLabel { text: qsTr("Behaviour") }
        ListRow {
            title: qsTr("Launch at login")
            subtitle: qsTr("Start quietly in the tray")
            hasSwitch: true
            checked: settings.launchAtLogin
            onToggled: (v) => settings.launchAtLogin = v
        }
        ListRow {
            title: qsTr("Connect automatically")
            subtitle: qsTr("When your earbuds connect to this computer")
            hasSwitch: true
            checked: settings.autoConnect
            onToggled: (v) => settings.autoConnect = v
        }
        ListRow {
            title: qsTr("Battery level in tray icon")
            subtitle: qsTr("The lower earbud, rounded to 5%")
            hasSwitch: true
            checked: settings.batteryInTray
            onToggled: (v) => settings.batteryInTray = v
        }
        ListRow {
            title: qsTr("Low battery notifications")
            subtitle: qsTr("At 20% and 10%")
            hasSwitch: true
            checked: settings.lowBatteryNotification
            onToggled: (v) => settings.lowBatteryNotification = v
        }
    }

    Card {
        SectionLabel { text: qsTr("Appearance") }
        Segmented {
            current: settings.appearance
            model: [{ label: qsTr("System"), value: "system" }, { label: qsTr("Dark"), value: "dark" }, { label: qsTr("Light"), value: "light" }]
            onPicked: (value) => settings.appearance = value
        }
        ListRow {
            title: qsTr("Use the %1 accent colour").arg(theme.systemName)
            subtitle: qsTr("Instead of Nothing red")
            hasSwitch: true
            checked: settings.systemAccent
            onToggled: (v) => settings.systemAccent = v
        }
    }

    Card {
        SectionLabel { text: qsTr("Official Nothing X app") }
        Txt {
            width: parent.width
            text: official.available
                ? qsTr("Using the look and device data of Nothing X %1. A newer version in your Downloads folder is picked up automatically.").arg(official.version)
                : qsTr("Not imported. Import the official app (.xapk or .apk) for Nothing's fonts, icons and product images.")
            size: 13
            color: theme.textMuted
        }
        Txt { visible: official.lastError !== ""; width: parent.width; text: official.lastError; size: 12; color: theme.accent }
        Pill {
            text: official.importing ? qsTr("Importing...") : official.available ? qsTr("Import a newer version") : qsTr("Import Nothing X app")
            enabled2: !official.importing
            onClicked: app.chooseOfficialApp()
        }
    }

    Card {
        SectionLabel { text: qsTr("Updates") }
        ListRow {
            title: qsTr("Nothing X %1").arg(updater.currentVersion)
            subtitle: updater.status !== "" ? updater.status : qsTr("Same version on Windows and Omarchy")
        }
        ListRow {
            title: qsTr("Update automatically")
            subtitle: qsTr("Check for new versions twice a day")
            hasSwitch: true
            checked: settings.autoUpdate
            onToggled: (v) => settings.autoUpdate = v
        }
        Row {
            spacing: 10
            Pill {
                visible: updater.updateAvailable && updater.canInstall
                text: qsTr("Update to %1").arg(updater.latestVersion)
                primary: true
                enabled2: !updater.busy
                onClicked: updater.install()
            }
            Pill {
                text: qsTr("Check for updates")
                enabled2: !updater.busy
                onClicked: updater.check()
            }
        }
        Txt {
            visible: updater.updateAvailable && !updater.canInstall
            width: parent.width
            text: qsTr("Running from a source checkout: update with git pull and bin/install.")
            size: 12
            color: theme.textMuted
        }
    }

    Card {
        SectionLabel { text: qsTr("About") }
        Txt {
            width: parent.width
            text: qsTr("An unofficial desktop companion for Nothing earbuds, built from the official Nothing X app's own protocol and design. Not affiliated with Nothing Technology Ltd.")
            size: 12
            color: theme.textMuted
        }
        Row {
            spacing: 10
            Pill { text: qsTr("Quit Nothing X"); onClicked: app.quit() }
        }
    }
}
