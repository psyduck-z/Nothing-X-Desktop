import QtQuick

// One of the bundled SVG icons, recoloured by the C++ image provider.
Image {
    property string name
    property color color: theme.text
    property int size: 20
    width: size
    height: size
    sourceSize: Qt.size(size * 2, size * 2)
    source: name ? "image://icon/" + name + "/" + color.toString().substring(1, 7) : ""
    smooth: true
    asynchronous: false
}
