#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T15:40:47
#
#-------------------------------------------------
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += core gui opengl
CONFIG   += c++11
CONFIG   += debug

INCLUDEPATH += ../PlayerLogic/
INCLUDEPATH += ../InputLibraryWrapper/
INCLUDEPATH += ../Common/
INCLUDEPATH += ../AudioOutput/

LIBS += ../../bin/libanmp.a  ../../../../Programme/lazyusf2/liblazyusf2.a ../../../../Programme/psflib/libpsflib.a ../../../../Programme/vgmstream/src/libvgmstream.a applets/analyzer/libanalyzer.a -lasound -lsndfile -lz -lcue -lvorbisfile -lmpg123 -lpthread -lm -lmad -lgme



TARGET = ANMP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        PlaylistView.cpp\
        ChannelConfigView.cpp\
        PlaylistModel.cpp \
        PlayheadSlider.cpp\
        configdialog.cpp \
    applets/analyzer/AnalyzerApplet.cpp \
    mainwindow.callback.cpp \
    mainwindow.slots.cpp \
    applets/channel/channelconfig.cpp \
    songinspector.cpp \
    ChannelConfigModel.cpp

HEADERS  += mainwindow.h PlaylistView.h PlaylistModel.h configdialog.h \
    applets/analyzer/AnalyzerApplet.h \
    PlayheadSlider.h \
    applets/channel/channelconfig.h \
    ChannelConfigView.h \
    songinspector.h \
    ChannelConfigModel.h

FORMS    += mainwindow.ui configdialog.ui \
    applets/analyzer/AnalyzerApplet.ui \
    applets/channel/channelconfig.ui \
    songinspector.ui \
    playcontrol.ui

DISTFILES +=

RESOURCES += \
    icons.qrc
