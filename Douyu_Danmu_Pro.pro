#-------------------------------------------------
#
# Project created by QtCreator 2016-04-12T19:41:37
#
#-------------------------------------------------

QT       += core gui network

QT += widgets

TARGET = Douyu_Danmu_Pro
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    networkaccess.cpp \
    jsonparse.cpp \
    douyutcpsocket.cpp \
    stringgenerator.cpp \
    singlecar.cpp \
    scarrankmodel.cpp

HEADERS  += mainwindow.h \
    networkaccess.h \
    jsonparse.h \
    danmuconfig.h \
    douyutcpsocket.h \
    stringgenerator.h \
    singlecar.h \
    scarrankmodel.h

FORMS    += mainwindow.ui

DISTFILES +=

RESOURCES += \
    img.qrc
