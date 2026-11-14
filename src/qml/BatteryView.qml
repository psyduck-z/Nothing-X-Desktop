import QtQuick

// One battery: dot-matrix percentage, a level bar, and a label.
Item {
    id: root
    property string label
    property string icon
    property int level: -1
    property bool charging: false
    property bool inEar: false
    implicitHeight: column.implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: 8

        Icon {
            name: root.icon
            size: 26
            anchors.horizontalCenter: parent.horizontalCenter
            color: root.level >= 0 ? theme.text : theme.textMuted
        }
        Item {
            width: parent.width
            height: digits.height
            DotText {
                id: digits
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.level >= 0 ? root.level + "%" : "--"
                dot: 2.6
                color: root.level >= 0 && root.level <= 20 ? "#D71921" : theme.text
            }
        }
        Rectangle {
            width: parent.width * 0.7
            height: 4
            radius: 2
            anchors.horizontalCenter: parent.horizontalCenter
            color: theme.surfaceRaised
            Rectangle {
                width: parent.width * Math.max(0, root.level) / 100
                height: parent.height
                radius: 2
                color: root.charging ? theme.accent : (root.level <= 20 ? "#D71921" : theme.text)
                Behavior on width { NumberAnimation { duration: 300 } }
            }
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5
            Rectangle {
                visible: root.charging || root.inEar
                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.charging ? theme.accent : theme.text
            }
            Text {
                text: root.charging ? qsTr("%1 · charging").arg(root.label) : root.label
                color: theme.textMuted
                font.pixelSize: 11
            }
        }
    }
}
