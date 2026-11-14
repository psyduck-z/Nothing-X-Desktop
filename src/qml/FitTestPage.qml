import QtQuick

// Ear tip fit test: plays a tone and reports the seal of each earbud.
ScrollPage {
    id: page
    title: qsTr("Ear tip fit test")
    readonly property var v: device.values
    readonly property bool running: v.fitRunning === true

    function verdict(r) {
        switch (r) {
        case 0: return qsTr("Good seal")
        case 1: return qsTr("Adjust the earbud or try another tip size")
        case 2: return qsTr("Not detected")
        }
        return "—"
    }

    Card {
        padding: 18
        spacing: 14
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: theme.text
            font.pixelSize: 13
            text: qsTr("Put both earbuds in, then start. A tone plays for about ten seconds while the earbuds measure how well the tips seal.")
        }
        PillButton {
            anchors.horizontalCenter: parent.horizontalCenter
            text: page.running ? qsTr("Testing…") : qsTr("Start test")
            icon: "play"
            enabled: !page.running
            onClicked: device.startFitTest()
        }
    }

    Card {
        visible: v.fitLeft !== undefined && v.fitLeft >= 0
        Row {
            width: parent.width
            Repeater {
                model: [{ label: qsTr("Left"), icon: "budleft", r: v.fitLeft }, { label: qsTr("Right"), icon: "budright", r: v.fitRight }]
                Column {
                    required property var modelData
                    width: parent.width / 2
                    spacing: 8
                    Icon { anchors.horizontalCenter: parent.horizontalCenter; name: modelData.icon; size: 30; color: modelData.r === 0 ? theme.text : theme.accent }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; color: theme.textMuted; font.pixelSize: 11 }
                    Text {
                        width: parent.width - 8
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: page.verdict(modelData.r)
                        color: theme.text
                        font.pixelSize: 13
                    }
                }
            }
        }
    }
}
