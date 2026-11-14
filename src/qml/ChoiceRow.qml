import QtQuick

// A setting with a list of choices that unfolds in place when clicked
// (no separate popup window, so it behaves the same in a layer-shell popup
// and on Windows).
Column {
    id: root
    property string title
    property string subtitle
    property var options: []       // [{ value, label }]
    property var current
    property bool expanded: false
    signal picked(var value)
    width: parent ? parent.width : 0
    opacity: enabled ? 1 : 0.5

    function labelFor(value) {
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === value)
                return options[i].label
        return "—"
    }

    Item {
        width: parent.width
        height: Math.max(44, texts.implicitHeight + 8)
        Column {
            id: texts
            anchors.left: parent.left
            anchors.right: valueText.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text { width: parent.width; text: root.title; color: theme.text; font.pixelSize: 14; wrapMode: Text.WordWrap }
            Text { width: parent.width; visible: text !== ""; text: root.subtitle; color: theme.textMuted; font.pixelSize: 11; wrapMode: Text.WordWrap }
        }
        Text {
            id: valueText
            anchors.right: chevron.left
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, parent.width * 0.45)
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            text: root.labelFor(root.current)
            color: theme.textMuted
            font.pixelSize: 13
        }
        Icon {
            id: chevron
            name: "chevron"
            size: 16
            color: theme.textMuted
            rotation: root.expanded ? 90 : 0
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            Behavior on rotation { NumberAnimation { duration: 120 } }
        }
        MouseArea {
            anchors.fill: parent
            enabled: root.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: root.expanded = !root.expanded
        }
    }
    Column {
        width: parent.width
        visible: root.expanded
        Repeater {
            model: root.options
            Item {
                required property var modelData
                readonly property bool selected: modelData.value === root.current
                width: parent.width
                height: 36
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: selected ? theme.surfaceRaised : "transparent"
                }
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: theme.text
                    font.pixelSize: 13
                }
                Rectangle {
                    visible: selected
                    width: 6; height: 6; radius: 3
                    color: theme.accent
                    anchors.right: parent.right
                    anchors.rightMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.picked(modelData.value)
                        root.expanded = false
                    }
                }
            }
        }
    }
}
