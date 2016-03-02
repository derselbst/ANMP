#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T15:40:47
#
#-------------------------------------------------
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += core gui
CONFIG   += c++11

INCLUDEPATH += ../PlayerLogic/
INCLUDEPATH += ../InputLibraryWrapper/
INCLUDEPATH += ../Common/
INCLUDEPATH += ../AudioOutput/

LIBS += ../build/libanmp.a -lasound -lsndfile



TARGET = ANMP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
