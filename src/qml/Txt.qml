import QtQuick

// Body text in the official app's UI face (Geist, from the imported app).
Text {
    property int size: 14
    property bool strong: false
    color: theme.text
    font.family: official.bodyFont
    font.pixelSize: size
    font.weight: strong ? Font.DemiBold : Font.Normal
    wrapMode: Text.WordWrap
    elide: Text.ElideRight
}
