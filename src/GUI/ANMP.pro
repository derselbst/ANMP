#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T15:40:47
#
#-------------------------------------------------
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += core gui
CONFIG   += c++11
CONFIG   += debug

INCLUDEPATH += ../PlayerLogic/
INCLUDEPATH += ../InputLibraryWrapper/
INCLUDEPATH += ../Common/
INCLUDEPATH += ../AudioOutput/

LIBS += ../../bin/libanmp.a  ../../../../Programme/lazyusf2/liblazyusf2.a ../../../../Programme/psflib/libpsflib.a ../../../../Programme/vgmstream/src/libvgmstream.a -lasound -lsndfile -lz -lcue -lvorbisfile -lmpg123 -lpthread -lm -lmad -lgme



TARGET = ANMP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        PlaylistView.cpp\
        PlaylistModel.cpp \
        configdialog.cpp \
    overlaycontrol.cpp

HEADERS  += mainwindow.h PlaylistView.h PlaylistModel.h configdialog.h \
    overlaycontrol.h

FORMS    += mainwindow.ui configdialog.ui \
    overlaycontrol.ui

DISTFILES +=
