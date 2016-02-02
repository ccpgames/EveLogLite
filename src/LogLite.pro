#-------------------------------------------------
#
# Project created by QtCreator 2015-07-02T15:30:36
#
#-------------------------------------------------

QT       += core gui network sql
win32 {
    QT += winextras
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LogLite
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    logmodel.cpp \
    logfilter.cpp \
    overlaylayout.cpp \
    logstatistics.cpp \
    logmonitorfilemodel.cpp \
    abstractlogmodel.cpp \
    logmap.cpp \
    settingsdialog.cpp \
    fixedheader.cpp \
    filtercondition.cpp \
    filterconditions.cpp \
    filterhighlight.cpp \
    filterdialog.cpp \
    highlightdialog.cpp \
    logview.cpp

HEADERS  += mainwindow.h \
    logmodel.h \
    logfilter.h \
    overlaylayout.h \
    logstatistics.h \
    abstractlogmodel.h \
    logmonitorfilemodel.h \
    logmap.h \
    settingsdialog.h \
    fixedheader.h \
    filtercondition.h \
    filterconditions.h \
    filterhighlight.h \
    filterdialog.h \
    highlightdialog.h \
    logview.h

FORMS    += mainwindow.ui \
    logstatistics.ui \
    settingsdialog.ui \
    filter.ui \
    filterhighlight.ui \
    highlights.ui

RESOURCES += \
    resources.qrc

CONFIG += c++11

VERSION = 1.4
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

QMAKE_TARGET_COMPANY = CCP
QMAKE_TARGET_PRODUCT = LogLite
QMAKE_TARGET_DESCRIPTION = LogLite server
QMAKE_TARGET_COPYRIGHT = Copyright CCP 2015

win32:RC_ICONS += resources/icon.ico
ICON = icon.icns

