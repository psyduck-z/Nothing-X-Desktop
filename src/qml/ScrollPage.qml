import QtQuick
import QtQuick.Controls

// A sub-page: a header, then content that scrolls under it.
Item {
    id: root
    property string title
    default property alias content: body.data
    signal back()

    PageHeader {
        id: header
        title: root.title
        onBack: root.back()
    }
    Flickable {
        anchors.top: header.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: body.implicitHeight + 16
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 4 }
        Column {
            id: body
            width: parent.width
            spacing: 12
        }
    }
}
