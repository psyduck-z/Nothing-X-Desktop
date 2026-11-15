import QtQuick

// Nothing's dot-matrix face (Ndot), for numbers and the big moments.
Text {
    property int size: 28
    color: theme.text
    font.family: official.dotFont
    font.pixelSize: size
}
