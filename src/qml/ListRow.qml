import QtQuick

// A settings row: optional icon, title and subtitle, then a switch, a value
// or a chevron. Clicking anywhere toggles the switch or opens the page.
Item {
    id: root
    property string icon
    property string title
    property string subtitle
    property string value
    property bool chevron: false
    property bool hasSwitch: false
    property bool checked: false
    property bool enabled2: true
    signal toggled(bool value)
    signal clicked()
    width: parent ? parent.width : 0
    implicitHeight: Math.max(48, texts.implicitHeight + 20)
    opacity: enabled2 ? 1 : 0.45

    Rectangle {
        anchors.fill: parent
        anchors.margins: -6
        radius: 12
        color: area.containsMouse && (root.chevron || root.hasSwitch) ? theme.surfaceRaised : "transparent"
        Behavior on color { ColorAnimation { duration: 100 } }
    }
    Icon {
        id: icon
        visible: root.icon !== "" && official.available
        path: root.icon
        size: 22
        color: theme.text
        anchors.verticalCenter: parent.verticalCenter
    }
    Column {
        id: texts
        anchors.left: icon.visible ? icon.right : parent.left
        anchors.leftMargin: icon.visible ? 14 : 0
        anchors.right: trailing.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Txt { width: parent.width; text: root.title; size: 15; wrapMode: Text.WordWrap }
        Txt {
            width: parent.width
            visible: root.subtitle !== ""
            text: root.subtitle
            size: 12
            color: theme.textMuted
        }
    }
    Row {
        id: trailing
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        Txt { visible: root.value !== ""; text: root.value; size: 13; color: theme.textMuted; anchors.verticalCenter: parent.verticalCenter; wrapMode: Text.NoWrap }
        NtSwitch {
            visible: root.hasSwitch
            checked: root.checked
            enabled2: root.enabled2
            onToggled: (v) => root.toggled(v)
        }
        Icon {
            visible: root.chevron
            path: "packages/nt_widget/assets/images/chevron_right.svg"
            color: theme.textMuted
            size: 18
            anchors.verticalCenter: parent.verticalCenter
        }
        Txt { visible: root.chevron && !official.available; text: "›"; size: 20; color: theme.textMuted }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled2 && (root.chevron || root.hasSwitch)
        cursorShape: Qt.PointingHandCursor
        onClicked: root.hasSwitch ? root.toggled(!root.checked) : root.clicked()
    }
}
