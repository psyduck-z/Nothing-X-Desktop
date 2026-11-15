import QtQuick

// The official app's confirmation before a change that turns something
// else off. ask(text, action) runs action() on confirm.
Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 100
    property var action
    property string confirmText: qsTr("Confirm")

    function ask(message, onConfirm, title) {
        heading.text = title || ""
        body.text = message
        action = onConfirm
        visible = true
    }

    Rectangle {
        anchors.fill: parent
        color: "#B3000000"
        MouseArea { anchors.fill: parent; onClicked: root.visible = false }
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        height: sheet.implicitHeight + 40
        radius: 24
        color: theme.surface
        Column {
            id: sheet
            x: 20
            y: 20
            width: parent.width - 40
            spacing: 16
            Headline { id: heading; width: parent.width; size: 20; visible: text !== ""; wrapMode: Text.WordWrap }
            Txt { id: body; width: parent.width; size: 14; color: theme.text }
            Row {
                anchors.right: parent.right
                spacing: 10
                Pill { text: qsTr("Cancel"); onClicked: root.visible = false }
                Pill {
                    text: root.confirmText
                    primary: true
                    onClicked: {
                        root.visible = false
                        if (root.action)
                            root.action()
                    }
                }
            }
        }
    }
}
