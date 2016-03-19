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

LIBS += ../../bin/libanmp.a  ../../../../Programme/lazyusf2/liblazyusf2.a ../../../../Programme/psflib/libpsflib.a -lasound -lsndfile -lz -lcue



TARGET = ANMP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        PlaylistView.cpp\
        PlaylistModel.cpp

HEADERS  += mainwindow.h PlaylistView.h PlaylistModel.h

FORMS    += mainwindow.ui
