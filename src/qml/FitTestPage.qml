import QtQuick

// The official Ear tip fit test: a tone plays while the earbuds measure how
// well each tip seals, then a result per earbud.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Ear tip fit test")
    readonly property var v: device.values
    readonly property bool running: v.fitRunning === true
    readonly property bool done: !running && v.fitLeft !== undefined && v.fitLeft !== -1

    function resultImage(r) {
        return r === 0 ? "assets/images/ear/ear_fit_status_yes.png" : r === 1 ? "assets/images/ear/ear_fit_status_no.png" : "assets/images/ear/ear_fit_status_error.png"
    }
    function resultText(r) {
        return r === 0 ? qsTr("Good seal") : r === 1 ? qsTr("Adjust fit") : qsTr("Test failed")
    }

    Txt {
        width: parent.width
        text: qsTr("Wear both earbuds and choose the right ear tip size for a secure fit and accurate results.")
        size: 13
        color: theme.textMuted
    }

    Card {
        Row {
            width: parent.width
            Repeater {
                model: [{ kind: "left", label: qsTr("Left"), r: v.fitLeft }, { kind: "right", label: qsTr("Right"), r: v.fitRight }]
                delegate: Column {
                    required property var modelData
                    width: parent.width / 2
                    spacing: 10
                    Item {
                        width: 120
                        height: 120
                        anchors.horizontalCenter: parent.horizontalCenter
                        // The pulsing ring of the official test.
                        Rectangle {
                            id: ring
                            anchors.centerIn: parent
                            width: 116; height: 116; radius: 58
                            color: "transparent"
                            border.width: 2
                            border.color: root.done ? (modelData.r === 0 ? theme.good : theme.accent) : theme.outline
                            SequentialAnimation on scale {
                                running: root.running
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.92; to: 1.04; duration: 700; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 1.04; to: 0.92; duration: 700; easing.type: Easing.InOutSine }
                            }
                        }
                        ProductImage { anchors.fill: parent; anchors.margins: 18; kind: modelData.kind }
                        Image {
                            visible: root.done && official.available
                            width: 32; height: 32
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            source: visible ? official.asset(root.resultImage(modelData.r)) : ""
                            sourceSize: Qt.size(64, 64)
                        }
                    }
                    Txt { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; size: 14; strong: true }
                    Txt {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: root.done
                        text: root.resultText(modelData.r)
                        size: 13
                        color: modelData.r === 0 ? theme.good : theme.accent
                    }
                }
            }
        }
        Txt {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.running ? qsTr("Testing in progress. Don't remove your earbuds.")
                : !root.done ? qsTr("Place your earbuds in your ears before starting the test.")
                : v.fitLeft === 0 && v.fitRight === 0 ? qsTr("Test completed. Great seal on both earbuds.")
                : v.fitLeft < 0 || v.fitRight < 0 ? qsTr("The test didn't finish. Make sure both earbuds are in your ears.")
                : qsTr("Adjust the %1 fit or try another ear tip size.").arg(v.fitLeft !== 0 && v.fitRight !== 0 ? qsTr("earbuds") : v.fitLeft !== 0 ? qsTr("left earbud") : qsTr("right earbud"))
            size: 13
        }
        Pill {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.done ? qsTr("Test again") : qsTr("Start test")
            primary: true
            enabled2: !root.running
            onClicked: device.startFitTest()
        }
    }

    Card {
        Repeater {
            model: [
                { t: qsTr("Improved audio quality"), d: qsTr("With a good seal, you will have deeper bass and richer lower frequencies.") },
                { t: qsTr("Optimised noise cancellation"), d: qsTr("Experience better noise cancellation with minimal sound leakage.") }
            ]
            delegate: Column {
                required property var modelData
                width: parent.width
                spacing: 2
                Txt { width: parent.width; text: modelData.t; size: 14; strong: true }
                Txt { width: parent.width; text: modelData.d; size: 12; color: theme.textMuted }
            }
        }
    }
}
