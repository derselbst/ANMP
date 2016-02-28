#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T15:40:47
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11

INCLUDEPATH += /home/tom/qtcreator/ANMP/ANMP/src/PlayerLogic/
INCLUDEPATH += /home/tom/qtcreator/ANMP/ANMP/src/InputLibraryWrapper/
INCLUDEPATH += /home/tom/qtcreator/ANMP/ANMP/src/Common/
INCLUDEPATH += /home/tom/qtcreator/ANMP/ANMP/src/AudioOutput/

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ANMP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
