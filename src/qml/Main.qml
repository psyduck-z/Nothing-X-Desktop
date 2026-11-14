import QtQuick
import QtQuick.Controls

// The tray popup. Everything the Nothing X phone app offers lives in this one
// small window: the home page shows the essentials and opens sub-pages for
// the rest.
Window {
    id: window
    width: 384
    height: Math.min(660, Screen.desktopAvailableHeight - 24)
    visible: false
    color: "transparent"
    title: "Nothing X"

    Rectangle {
        id: frame
        anchors.fill: parent
        radius: app.platform === "windows" ? 8 : 22
        color: theme.background
        border.width: 1
        border.color: theme.outline
        clip: true

        StackView {
            id: stack
            anchors.fill: parent
            anchors.margins: 16
            initialItem: home
            focus: true

            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 }
                    NumberAnimation { property: "x"; from: 24; to: 0; duration: 180; easing.type: Easing.OutCubic }
                }
            }
            pushExit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
            popEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 } }
            popExit: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140 }
                    NumberAnimation { property: "x"; from: 0; to: 24; duration: 160 }
                }
            }
        }
    }

    Component {
        id: home
        HomePage { onOpen: (page) => window.open(page) }
    }

    function closeAll() { stack.pop(null) }

    function open(name) {
        const pages = {
            "devices": "DevicesPage.qml",
            "settings": "AppSettingsPage.qml",
            "eq": "EqualizerPage.qml",
            "gestures": "GesturesPage.qml",
            "sound": "SoundPage.qml",
            "mic": "MicPage.qml",
            "fit": "FitTestPage.qml",
            "find": "FindPage.qml",
            "dual": "DualConnectionPage.qml",
            "about": "DeviceInfoPage.qml"
        }
        const page = stack.push(Qt.resolvedUrl(pages[name]))
        page.back.connect(() => stack.pop())
    }

    // Leave sub-pages when the earbuds go away, so the popup never shows
    // controls for a device that isn't there.
    Connections {
        target: device
        function onStateChanged() {
            if (device.state !== 2 && stack.depth > 1 && stack.currentItem.objectName !== "devices"
                    && stack.currentItem.objectName !== "settings")
                stack.pop(null)
        }
    }

    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: stack.depth > 1 ? stack.pop() : app.hidePopup()
    }
    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: app.quit()
    }
}
