import QtQuick

// Pinch controls for each earbud and the case's Talk button.
ScrollPage {
    id: page
    title: qsTr("Controls")
    property int side: 2 // 2 left, 3 right, 4 case
    // Read so bindings re-run when gestures change.
    readonly property var all: device.gestures

    function action(type) { all; return device.gestureAction(page.side, type) }
    function isNoiseControl(a) { return a === 10 || a === 20 || a === 21 || a === 22 }

    readonly property var noiseControlOption: ({ value: "nc", label: qsTr("Noise control") })
    readonly property var media: [
        { value: 8, label: qsTr("Skip back") },
        { value: 9, label: qsTr("Skip forward") },
        { value: 11, label: qsTr("Voice assistant") }
    ]

    Segmented {
        options: device.features.includes("caseButton")
            ? [{ value: 2, label: qsTr("Left") }, { value: 3, label: qsTr("Right") }, { value: 4, label: qsTr("Case") }]
            : [{ value: 2, label: qsTr("Left") }, { value: 3, label: qsTr("Right") }]
        current: page.side
        onPicked: (s) => page.side = s
    }

    // Earbuds ---------------------------------------------------------------
    Card {
        visible: page.side !== 4
        spacing: 2
        Item {
            width: parent.width
            height: 44
            Column {
                anchors.verticalCenter: parent.verticalCenter
                Text { text: qsTr("Pinch"); color: theme.text; font.pixelSize: 14 }
                Text { text: qsTr("Play, pause, answer a call"); color: theme.textMuted; font.pixelSize: 11 }
            }
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Fixed")
                color: theme.textMuted
                font.pixelSize: 13
            }
        }
        ChoiceRow {
            title: qsTr("Double pinch")
            options: page.media
            current: page.action(2)
            onPicked: (a) => device.setGesture(page.side, 2, a)
        }
        ChoiceRow {
            title: qsTr("Triple pinch")
            options: page.media.concat([{ value: 1, label: qsTr("No action") }])
            current: page.action(3)
            onPicked: (a) => device.setGesture(page.side, 3, a)
        }
        ChoiceRow {
            id: hold
            title: qsTr("Pinch and hold")
            options: [page.noiseControlOption,
                      { value: 18, label: qsTr("Volume up") },
                      { value: 19, label: qsTr("Volume down") },
                      { value: 11, label: qsTr("Voice assistant") },
                      { value: 29, label: qsTr("Mic on/off") },
                      { value: 1, label: qsTr("No action") }]
            current: page.isNoiseControl(page.action(7)) ? "nc" : page.action(7)
            onPicked: (a) => device.setGesture(page.side, 7, a === "nc" ? 22 : a)
        }
        NoiseCycleRow {
            visible: page.isNoiseControl(page.action(7))
            current: page.action(7)
            onPicked: (a) => device.setGesture(page.side, 7, a)
        }
        ChoiceRow {
            title: qsTr("Double pinch and hold")
            options: [page.noiseControlOption,
                      { value: 18, label: qsTr("Volume up") },
                      { value: 19, label: qsTr("Volume down") },
                      { value: 11, label: qsTr("Voice assistant") },
                      { value: 1, label: qsTr("No action") }]
            current: page.isNoiseControl(page.action(9)) ? "nc" : page.action(9)
            onPicked: (a) => device.setGesture(page.side, 9, a === "nc" ? 22 : a)
        }
        NoiseCycleRow {
            visible: page.isNoiseControl(page.action(9))
            current: page.action(9)
            onPicked: (a) => device.setGesture(page.side, 9, a)
        }
    }

    // Case Talk button ---------------------------------------------------------
    Card {
        visible: page.side === 4
        spacing: 2
        ChoiceRow {
            title: qsTr("Press and hold")
            subtitle: qsTr("The Talk button on the case")
            options: [{ value: 33, label: qsTr("Essential Space") },
                      { value: 11, label: qsTr("Voice assistant") },
                      { value: 1, label: qsTr("No action") }]
            current: page.action(7)
            onPicked: (a) => device.setGesture(4, 7, a)
        }
        Item {
            width: parent.width
            height: 44
            Column {
                anchors.verticalCenter: parent.verticalCenter
                Text { text: qsTr("Double press"); color: theme.text; font.pixelSize: 14 }
                Text { text: qsTr("Super Mic on/off"); color: theme.textMuted; font.pixelSize: 11 }
            }
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Fixed")
                color: theme.textMuted
                font.pixelSize: 13
            }
        }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        color: theme.textMuted
        font.pixelSize: 11
        text: qsTr("Changes are saved on the earbuds and work with every device they connect to.")
    }
}
