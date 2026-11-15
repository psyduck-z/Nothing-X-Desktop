QT += core gui widgets qml quick quickcontrols2 svg bluetooth network testlib
CONFIG += c++20 console
TARGET = ui_tests
DEFINES += NOTHINGX_VERSION=\\\"0.1.0\\\" NOTHINGX_REPO=\\\"psyduck-z/Nothing-X-Desktop\\\"
include(../../src/sources.pri)
SOURCES += ui_tests.cpp
RESOURCES += ../../src/resources.qrc
unix:!macx:exists(/usr/include/LayerShellQt/window.h) {
    DEFINES += HAVE_LAYERSHELLQT
    LIBS += -lLayerShellQtInterface
}
win32: LIBS += -ldwmapi
