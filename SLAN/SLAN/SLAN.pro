QT -= gui

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
QMAKE_CC = g++
CONFIG += depend_includepath
CONFIG += c++11
CONFIG(debug, debug|release): QMAKE_CXXFLAGS_WARN_ON += -fpermissive
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES -= UNICODE
DEFINES += _MBCS


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#SOURCES += \
#HEADERS += \
#    ../../server/slan/private/prvOsDependent.h \
#    ../../server/slan/private/prvSBCommon.h
win32 {
#INCLUDEPATH += $$PWD/../../Central/MTS_utils/repeater/
INCLUDEPATH += C:\git\Central\MTS_utils\repeater
INCLUDEPATH += C:\git\Central\MTS_utils\repeater\sagent\exp
INCLUDEPATH += C:\git\Central\MTS_utils\repeater\common
} else {
INCLUDEPATH += $$PWD/../../../../git/pss_unix_utils/
INCLUDEPATH += /local/store/git/Central/pss_unix_utils/
}
# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

HEADERS +=

win32 {
HEADERS +=
SOURCES += C:\git\Central\MTS_utils\repeater\slan\slan.c
SOURCES += C:\git\Central\MTS_utils\repeater\sagent\sagent.c
SOURCES += C:\git\Central\MTS_utils\repeater\common\stubs\stubs.c
} else {
SOURCES += \
    /local/store/git/Central/pss_unix_utils/slanLib.c
}
SOURCES +=stubs.c
