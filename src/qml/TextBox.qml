import QtQuick

// A single-line text field in the Nothing look; commits on Enter or when
// focus leaves.
Rectangle {
    id: root
    property alias text: input.text
    property string placeholder
    property bool mono: false
    signal committed(string text)
    implicitHeight: 44
    radius: 12
    color: theme.surfaceRaised
    border.width: input.activeFocus ? 1 : 0
    border.color: theme.text
    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        verticalAlignment: TextInput.AlignVCenter
        color: theme.text
        selectionColor: theme.accent
        font.family: root.mono ? official.monoFont : official.bodyFont
        font.pixelSize: 13
        clip: true
        selectByMouse: true
        onAccepted: { root.committed(text); focus = false }
        onActiveFocusChanged: if (!activeFocus) root.committed(text)
    }
    Txt {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        visible: input.text === "" && !input.activeFocus
        text: root.placeholder
        size: 13
        color: theme.textDim
    }
}
