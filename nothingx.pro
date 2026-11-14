# Nothing X Desktop: one source tree, two targets.
#
# Linux (Omarchy/Hyprland) and Windows build the exact same sources and QML;
# the only platform code is how the popup is placed next to the tray and how
# "launch at login" is registered (see src/popupplacer.cpp, src/autostart.cpp).

QT += core gui widgets qml quick quickcontrols2 svg bluetooth network

CONFIG += c++20 release
TARGET = nothingx
TEMPLATE = app
VERSION = 0.1.0
DEFINES += NOTHINGX_VERSION=\\\"$$VERSION\\\"

HEADERS += \
    src/app.h \
    src/autostart.h \
    src/bttransport.h \
    src/demotransport.h \
    src/device.h \
    src/devicescanner.h \
    src/iconprovider.h \
    src/models.h \
    src/popupplacer.h \
    src/protocol.h \
    src/settings.h \
    src/theme.h \
    src/transport.h \
    src/trayicon.h

SOURCES += \
    src/main.cpp \
    src/app.cpp \
    src/autostart.cpp \
    src/bttransport.cpp \
    src/demotransport.cpp \
    src/device.cpp \
    src/devicescanner.cpp \
    src/iconprovider.cpp \
    src/models.cpp \
    src/popupplacer.cpp \
    src/protocol.cpp \
    src/settings.cpp \
    src/theme.cpp \
    src/trayicon.cpp

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
