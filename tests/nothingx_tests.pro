QT += core gui widgets quick testlib
CONFIG += c++20 testcase
TARGET = nothingx_tests
TEMPLATE = app
DEFINES += NOTHINGX_VERSION=\\\"test\\\"

INCLUDEPATH += ../src
HEADERS += \
    ../src/autostart.h ../src/demotransport.h ../src/device.h ../src/models.h \
    ../src/popupplacer.h ../src/protocol.h ../src/settings.h ../src/theme.h ../src/transport.h
SOURCES += \
    nothingx_tests.cpp \
    ../src/autostart.cpp ../src/demotransport.cpp ../src/device.cpp ../src/models.cpp \
    ../src/popupplacer.cpp ../src/protocol.cpp ../src/settings.cpp ../src/theme.cpp

unix:!macx:exists(/usr/include/LayerShellQt/window.h) {
    DEFINES += HAVE_LAYERSHELLQT
    LIBS += -lLayerShellQtInterface
}
win32: LIBS += -ldwmapi
