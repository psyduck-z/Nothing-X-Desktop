import QtQuick

// A rounded surface that groups related controls.
Rectangle {
    default property alias content: column.data
    property alias spacing: column.spacing
    property int padding: 14
    width: parent ? parent.width : 0
    implicitHeight: column.implicitHeight + padding * 2
    radius: 18
    color: theme.surface

    Column {
        id: column
        x: parent.padding
        y: parent.padding
        width: parent.width - parent.padding * 2
        spacing: 10
    }
}
