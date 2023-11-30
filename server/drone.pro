TEMPLATE = app
CONFIG += qt ver_info c++11
addon: CONFIG -= ver_info
QT += network script xml
QT -= gui

DEFINES -= Q_OS_WIN32

linux*:system(grep -q IFLA_STATS64 /usr/include/linux/if_link.h): \
    DEFINES += HAVE_IFLA_STATS64
INCLUDEPATH += "../common"
INCLUDEPATH += "../rpc"
INCLUDEPATH += C:\PROTO\protobuf-21.6\src
INCLUDEPATH += C:\PROTO\WpdPack\Include
INCLUDEPATH += C:\git\Central\MTS_utils\repeater\sagent\exp
win32 {
    # Support Windows Vista and above only
    DEFINES += WIN32_LEAN_AND_MEAN NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
    DEFINES += HAVE_REMOTE WPCAP
    CONFIG += console
    QMAKE_LFLAGS += -static
    LIBS += -L"C:/PROTO/WpdPack/Lib" -lwpcap -lpacket -liphlpapi
    LIBS += -L"C:\git\NewOstinato\SLAN\SLAN\debug" -lSLAN
    CONFIG(debug, debug|release) {
        LIBS += -L"../common/debug" -lostproto
        LIBS += -L"../rpc/debug" -lpbrpc
        POST_TARGETDEPS += \
            "../common/debug/libostproto.a" \
            "../rpc/debug/libpbrpc.a"
    } else {
        LIBS += -L"../common/release" -lostproto
        LIBS += -L"../rpc/release" -lpbrpc
        POST_TARGETDEPS += \
            "../common/release/libostproto.a" \
            "../rpc/release/libpbrpc.a"
    }
} else {
    LIBS += -lpcap
    LIBS += -L"../common" -lostproto
    LIBS += -L"../rpc" -lpbrpc
    LIBS += -L"/local/store/git/NewOstinato/SLAN/SLAN/" -lSLAN
    POST_TARGETDEPS += "../common/libostproto.a" "../rpc/libpbrpc.a"
}
linux {
    INCLUDEPATH += "/usr/include/libnl3"
    INCLUDEPATH += "/local/store/git/NewOstinato/SLAN/SLAN/"
    LIBS += -lnl-3 -lnl-route-3
}
LIBS += -lm
#LIBS += -lprotobuf
LIBS += -L"C:/PROTO/protobuf-21.6/INSTALL/lib" -lprotobuf
HEADERS += drone.h \
    ITxThread.h \
    pcaptxthread.h \
    pcaptransmitter.h \
    myservice.h \
    slanTxThread.h \
    slantransmitter.h \
    streamtiming.h \
    slanport.h \
    GlobalVC.h

SOURCES += \
    devicemanager.cpp \
    device.cpp \
    emuldevice.cpp \
    drone_main.cpp \
    drone.cpp \
    portmanager.cpp \
    abstractport.cpp \
    pcapport.cpp \
    pcapsession.cpp \
    pcaptransmitter.cpp \
    pcaprxstats.cpp \
    pcaptxstats.cpp \
    pcaptxthread.cpp \
    pcaptxttagstats.cpp \
    bsdhostdevice.cpp \
    bsdport.cpp \
    linuxhostdevice.cpp \
    linuxport.cpp \
    linuxutils.cpp \
    params.cpp \
    slanport.cpp \
    SlanCfgReader.cpp \
    slantransmitter.cpp \
    slanTxThread.cpp \
    streamtiming.cpp \
    turbo.cpp \
    winhostdevice.cpp \
    winpcapport.cpp \
    win-gettimeofday.cpp

SOURCES += myservice.cpp 
SOURCES += pcapextra.cpp 
SOURCES += packetbuffer.cpp

QMAKE_DISTCLEAN += object_script.*

include (../install.pri)
include (../version.pri)
include (../options.pri)
