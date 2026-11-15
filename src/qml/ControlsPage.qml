import QtQuick
import "Strings.js" as S

// The official Controls page: each earbud's pinch gestures and the case's
// TALK button, as the model's configuration lists them.
ScrollPage {
    id: root
    property var ui
    title: qsTr("Controls")
    property int side: 2 // 2 left, 3 right, 4 case

    // The earbuds (and case) at the top, the selected one lit.
    Row {
        width: parent.width
        height: 130
        Repeater {
            model: device.features.caseButton
                ? [{ kind: "left", side: 2, label: qsTr("Left") }, { kind: "right", side: 3, label: qsTr("Right") }, { kind: "box", side: 4, label: qsTr("Case") }]
                : [{ kind: "left", side: 2, label: qsTr("Left") }, { kind: "right", side: 3, label: qsTr("Right") }]
            delegate: Item {
                required property var modelData
                width: parent.width / (device.features.caseButton ? 3 : 2)
                height: parent.height
                opacity: root.side === modelData.side ? 1 : 0.35
                Behavior on opacity { NumberAnimation { duration: 150 } }
                ProductImage {
                    anchors.fill: parent
                    anchors.bottomMargin: 24
                    kind: modelData.kind
                }
                Txt {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: modelData.label
                    size: 13
                    strong: root.side === modelData.side
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.side = modelData.side }
            }
        }
    }

    Segmented {
        current: root.side
        model: device.features.caseButton
            ? [{ label: qsTr("Left"), value: 2 }, { label: qsTr("Right"), value: 3 }, { label: qsTr("Talk button"), value: 4 }]
            : [{ label: qsTr("Left"), value: 2 }, { label: qsTr("Right"), value: 3 }]
        onPicked: (value) => root.side = value
    }

    Card {
        spacing: 4
        Repeater {
            model: device.gestureGroups.filter((g) => (root.side === 4) === (g.device === 4))
            delegate: Column {
                required property var modelData
                required property int index
                width: parent.width
                readonly property var others: device.gestureGroups.filter((g) => (root.side === 4) === (g.device === 4))
                readonly property bool newGroup: index === 0 || others[index - 1].groupName !== modelData.groupName
                readonly property bool fixed: !modelData.custom || modelData.options.every((o) => !o.custom)
                readonly property int current: { device.gestures; return device.gestureAction(root.side, modelData.gesture) }
                SectionLabel {
                    visible: modelData.groupName !== "" && parent.newGroup
                    text: qsTr(S.t(modelData.groupName))
                    topPadding: 12
                    bottomPadding: 4
                }
                ListRow {
                    icon: S.gestureIcon(modelData.imageKey)
                    title: qsTr(S.t(modelData.nameKey))
                    subtitle: parent.fixed
                        ? modelData.options.map((o) => o.keys.map((k) => qsTr(S.t(k))).join(" · ")).join(" · ")
                        : S.isNoiseCycle(parent.current) ? qsTr("Noise control") + ": " + S.cycleSummary(parent.current)
                        : qsTr(S.action(parent.current, root.side))
                    chevron: !parent.fixed
                    onClicked: root.ui.open("gesture", { part: root.side, gesture: modelData.gesture })
                }
            }
        }
    }

    Pill {
        text: qsTr("Reset")
        anchors.horizontalCenter: parent.horizontalCenter
        onClicked: root.ui.confirm(qsTr("Are you sure you want to reset the controls?"), () => device.resetGestures())
    }
}
