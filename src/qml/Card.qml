import QtQuick

// A rounded surface; children stack vertically with the card's padding.
Rectangle {
    id: card
    default property alias content: column.data
    property int padding: 16
    property alias spacing: column.spacing
    width: parent ? parent.width : 0
    implicitHeight: column.implicitHeight + padding * 2
    radius: 20
    color: theme.surface
    Column {
        id: column
        x: card.padding
        y: card.padding
        width: card.width - card.padding * 2
        spacing: 12
    }
}
