# Nothing X Desktop: one source tree, two targets.
#
# Linux (Omarchy/Hyprland) and Windows build the exact same sources and QML;
# the only platform code is how the popup is placed next to the tray, how
# "launch at login" is registered and how an update is installed.

QT += core gui widgets qml quick quickcontrols2 svg bluetooth network

CONFIG += c++20 release
TARGET = nothingx
TEMPLATE = app
# The official Nothing X version this build follows, then the desktop build:
# 3.10.3.1 is the first desktop build of Nothing X 3.10.3. Windows and
# Omarchy always ship the same number.
VERSION = 3.10.3.1
DEFINES += NOTHINGX_VERSION=\\\"$$VERSION\\\" NOTHINGX_REPO=\\\"psyduck-z/Nothing-X-Desktop\\\"

include(src/sources.pri)
SOURCES += src/main.cpp
RESOURCES += src/resources.qrc

# On Wayland the popup is a layer-shell surface anchored to the bar's corner,
# because regular Wayland windows cannot choose their own position.
unix:!macx {
    exists(/usr/include/LayerShellQt/window.h) {
        DEFINES += HAVE_LAYERSHELLQT
        LIBS += -lLayerShellQtInterface
    } else {
        warning("layer-shell-qt not found: the popup will open as a normal window on Wayland")
    }
    target.path = /usr/bin
    INSTALLS += target
}

win32 {
    RC_ICONS = packaging/windows/nothingx.ico
    QMAKE_TARGET_DESCRIPTION = "Nothing X"
    QMAKE_TARGET_PRODUCT = "Nothing X"
    LIBS += -ldwmapi
    DEFINES += NOMINMAX WIN32_LEAN_AND_MEAN
}
