import QtQuick

// A labelled setting with a switch and an optional explanation underneath.
Item {
    id: root
    property string icon
    property string title
    property string subtitle
    property bool checked
    signal toggled(bool checked)
    width: parent ? parent.width : 0
    implicitHeight: Math.max(44, texts.implicitHeight + 8)

    Icon {
        id: glyph
        visible: root.icon !== ""
        name: root.icon
        anchors.verticalCenter: parent.verticalCenter
        opacity: root.enabled ? 1 : 0.4
    }
    Column {
        id: texts
        anchors.left: glyph.visible ? glyph.right : parent.left
        anchors.leftMargin: glyph.visible ? 12 : 0
        anchors.right: sw.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        opacity: root.enabled ? 1 : 0.4
        Text {
            width: parent.width
            text: root.title
            color: theme.text
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }
        Text {
            width: parent.width
            visible: text !== ""
            text: root.subtitle
            color: theme.textMuted
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
    PillSwitch {
        id: sw
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        checked: root.checked
        enabled: root.enabled
        onToggled: (value) => root.toggled(value)
    }
}
