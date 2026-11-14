import QtQuick

// The small uppercase label above a group, in Nothing's style.
Text {
    width: parent ? parent.width : implicitWidth
    color: theme.textMuted
    font.pixelSize: 11
    font.letterSpacing: 1.6
    font.capitalization: Font.AllUppercase
    elide: Text.ElideRight
    topPadding: 6
}
