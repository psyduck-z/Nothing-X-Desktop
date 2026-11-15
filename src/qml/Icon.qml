import QtQuick

// One of the official app's icons, tinted like the official app tints them.
// `path` is the asset path inside the imported app; nothing is drawn while
// no official app is imported.
Item {
    id: root
    property string path
    property color color: theme.text
    property bool tint: true
    property int size: 24
    implicitWidth: size
    implicitHeight: size
    Image {
        anchors.fill: parent
        visible: root.path !== "" && official.available
        source: visible ? "image://official/" + (root.tint ? root.color.toString().slice(1) : "-") + "/" + root.path : ""
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: false
    }
}
