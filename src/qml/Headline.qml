import QtQuick

// Titles in NType 82, Nothing's headline typeface.
Text {
    property int size: 24
    color: theme.text
    font.family: official.headlineFont
    font.pixelSize: size
    elide: Text.ElideRight
}
