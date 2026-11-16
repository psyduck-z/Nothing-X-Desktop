import QtQuick
import "Strings.js" as S

// The official operation list for one gesture: pick what it does.
ScrollPage {
    id: root
    property var ui
    property int part: 2
    property int gesture: 7
    readonly property var entry: {
        for (const g of device.gestureGroups)
            if (((root.part === 4) === (g.device === 4)) && g.gesture === root.gesture && g.custom)
                return g
        return { nameKey: "", options: [] }
    }
    readonly property int current: { device.gestures; return device.gestureAction(root.part, root.gesture) }
    title: qsTr(S.t(entry.nameKey))

    // The action the computer answers: News on the earbuds, Essential
    // Space on the case's TALK button.
    function isDesktop(action) { return action === (root.part === 4 ? 33 : 31) }

    SectionLabel {
        text: root.part === 2 ? qsTr("Left earbud") : root.part === 3 ? qsTr("Right earbud") : qsTr("Talk button")
    }

    Card {
        spacing: 2
        Repeater {
            // Phone-only actions (News, Essential Space) need a Nothing phone;
            // they are listed only while one of them is set, or where the
            // desktop takes them over: News on the earbuds, Essential Space
            // on the case's TALK button, both shown as "Desktop assistant".
            model: root.entry.options.filter((o) => !o.phoneOnly || root.isDesktop(o.action) || o.action === root.current)
            delegate: Column {
                required property var modelData
                required property int index
                width: parent.width
                readonly property var all: root.entry.options.filter((o) => !o.phoneOnly || root.isDesktop(o.action) || o.action === root.current)
                readonly property bool newGroup: index === 0 || all[index - 1].group !== modelData.group
                readonly property bool isCycle: modelData.action === 10
                readonly property bool selected: isCycle ? S.isNoiseCycle(root.current) : modelData.action === root.current
                SectionLabel {
                    visible: parent.newGroup && modelData.group !== "" && modelData.group !== "empty"
                    text: qsTr(S.t(modelData.group))
                    topPadding: index === 0 ? 0 : 12
                    bottomPadding: 4
                }
                Item {
                    width: parent.width
                    height: 50
                    opacity: modelData.custom ? 1 : 0.6
                    Txt {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.isDesktop(modelData.action) ? qsTr("Desktop assistant")
                            : modelData.keys.map((k) => qsTr(S.t(k))).join(" · ")
                        size: 15
                        width: parent.width - 40
                    }
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: modelData.custom
                        width: 22; height: 22; radius: 11
                        color: parent.parent.selected ? theme.text : "transparent"
                        border.width: 2
                        border.color: parent.parent.selected ? theme.text : theme.outline
                        Rectangle { anchors.centerIn: parent; width: 8; height: 8; radius: 4; color: theme.surface; visible: parent.parent.parent.selected }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: modelData.custom
                        cursorShape: Qt.PointingHandCursor
                        onClicked: device.setGesture(root.part, root.gesture,
                                                      parent.parent.isCycle ? (S.isNoiseCycle(root.current) ? root.current : 22) : modelData.action)
                    }
                }
                // Which modes the noise-control cycle goes through.
                Column {
                    visible: parent.isCycle && parent.selected
                    width: parent.width
                    leftPadding: 12
                    bottomPadding: 8
                    Repeater {
                        model: [
                            { key: "anc", label: qsTr("Noise Cancellation") },
                            { key: "transparency", label: qsTr("Transparency") },
                            { key: "off", label: qsTr("Off") }
                        ]
                        delegate: Item {
                            required property var modelData
                            readonly property var modes: S.cycleModes(root.current)
                            width: parent.width - 12
                            height: 40
                            Txt { anchors.verticalCenter: parent.verticalCenter; text: modelData.label; size: 14; color: theme.textMuted }
                            Rectangle {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 20; height: 20; radius: 6
                                color: parent.modes[modelData.key] ? theme.text : "transparent"
                                border.width: 2
                                border.color: parent.modes[modelData.key] ? theme.text : theme.outline
                                Txt { anchors.centerIn: parent; text: "✓"; size: 12; strong: true; color: theme.surface; visible: parent.parent.modes[modelData.key] }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    const m = Object.assign({}, parent.modes)
                                    m[modelData.key] = !m[modelData.key]
                                    const code = S.cycleCode(m)
                                    if (code === 0)
                                        root.ui.toast(qsTr("Select at least two modes."))
                                    else
                                        device.setGesture(root.part, root.gesture, code)
                                }
                            }
                        }
                    }
                }
                Txt {
                    readonly property string command: settings.assistantCommand !== "" ? settings.assistantCommand : app.assistantFallback
                    visible: root.isDesktop(modelData.action) && parent.selected
                    width: parent.width
                    text: (root.part === 4 ? qsTr("Press and hold the TALK button to start a conversation; do it again to end it.") + " " : "")
                        + (command !== "" ? qsTr("Runs %1").arg(command) : qsTr("Choose what this runs in App settings › Assistant."))
                    size: 12
                    color: command !== "" ? theme.textMuted : theme.accent
                    bottomPadding: 8
                }
                Txt {
                    visible: modelData.tipKey !== "" && !root.isDesktop(modelData.action) && (parent.selected || !modelData.custom)
                    width: parent.width
                    text: qsTr(S.t(modelData.tipKey))
                    size: 12
                    color: theme.textMuted
                    bottomPadding: 8
                }
            }
        }
    }

    Txt {
        visible: root.entry.options.some((o) => o.chooser)
        width: parent.width
        text: qsTr("Voice assistant opens your phone's or computer's default assistant.")
        size: 12
        color: theme.textMuted
    }
}
