import QtQuick
import QtQuick.Controls

// A sub-page: header, then a scrolling column of content.
Item {
    id: root
    property string title
    default property alias content: column.data
    property alias spacing: column.spacing
    signal back()
    signal open(string page)

    PageHeader {
        id: header
        title: root.title
        onBack: root.back()
    }
    Flickable {
        id: flick
        anchors.top: header.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: column.implicitHeight + 16
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { width: 4; policy: ScrollBar.AsNeeded }
        Column {
            id: column
            width: flick.width
            spacing: 12
        }
    }
}
