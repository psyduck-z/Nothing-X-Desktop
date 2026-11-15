import QtQuick
import QtQuick.Controls

// The tray popup: everything the official Nothing X app offers for the
// connected earbuds, one page at a time.
Window {
    id: window
    width: 384
    height: Math.min(680, Screen.desktopAvailableHeight - 24)
    visible: false
    color: "transparent"
    title: "Nothing X"

    // Every page, for the screenshots developer aid (App::takeScreenshots).
    readonly property var pageNames: ["home", "eq", "eqAdvanced", "controls", "gesture", "supermic", "settings", "hqa",
        "dual", "fit", "find", "about", "devices", "app"]
    readonly property var pageFiles: ({
        "eq": "EqualizerPage.qml",
        "eqAdvanced": "EqualizerPage.qml",
        "controls": "ControlsPage.qml",
        "gesture": "GesturePage.qml",
        "supermic": "SuperMicPage.qml",
        "settings": "DeviceSettingsPage.qml",
        "hqa": "HighQualityAudioPage.qml",
        "dual": "DualConnectionPage.qml",
        "fit": "FitTestPage.qml",
        "find": "FindPage.qml",
        "about": "AboutPage.qml",
        "devices": "DevicesPage.qml",
        "app": "AppSettingsPage.qml"
    })

    // What pages use to talk to the window.
    QtObject {
        id: bridge
        function open(name, props) { window.open(name, props) }
        function back() { if (stack.depth > 1) stack.pop() }
        function confirm(message, action, title) { sheet.ask(message, action, title) }
        // Runs action now, or after the official app's warning when the
        // change turns something else off.
        function guarded(setting, value, action) {
            const warning = device.conflict(setting, value)
            if (warning !== "")
                sheet.ask(warning, action)
            else
                action()
        }
        function toast(message) { toast.show(message) }
    }

    Rectangle {
        id: frame
        anchors.fill: parent
        radius: app.platform === "windows" ? 8 : 24
        color: theme.background
        border.width: 1
        border.color: theme.outline
        clip: true

        StackView {
            id: stack
            anchors.fill: parent
            anchors.margins: 16
            anchors.bottomMargin: 0
            initialItem: home
            focus: true
            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 }
                    NumberAnimation { property: "x"; from: 28; to: 0; duration: 200; easing.type: Easing.OutCubic }
                }
            }
            pushExit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
            popEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 } }
            popExit: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140 }
                    NumberAnimation { property: "x"; from: 0; to: 28; duration: 160 }
                }
            }
        }

        Toast { id: toast }
        ConfirmSheet { id: sheet }
    }

    Component {
        id: home
        HomePage { ui: bridge }
    }

    function closeAll() { stack.pop(null) }

    function open(name, props) {
        if (name === "home")
            return
        const p = Object.assign({ ui: bridge }, props || {})
        if (name === "eqAdvanced")
            p.startAdvanced = true
        if (name === "gesture" && p.part === undefined)
            Object.assign(p, { part: 2, gesture: 7 })
        const page = stack.push(Qt.resolvedUrl(pageFiles[name]), p)
        if (page) {
            page.objectName = name
            page.back.connect(() => stack.pop())
        }
    }

    Connections {
        target: device
        function onNotice(message) { toast.show(message) }
        // Leave device pages when the earbuds go away.
        function onStateChanged() {
            const keep = ["devices", "app"]
            if (device.state !== 2 && stack.depth > 1 && keep.indexOf(stack.currentItem.objectName) < 0)
                stack.pop(null)
        }
    }
    Connections {
        target: official
        function onImportFinished(ok, message) { toast.show(message) }
    }

    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: sheet.visible ? sheet.visible = false : stack.depth > 1 ? stack.pop() : app.hidePopup()
    }
    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: app.quit()
    }

    // Drop an official .xapk/.apk on the popup to import it.
    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (drop.hasUrls)
                official.importFile(drop.urls[0].toString())
        }
    }
}
