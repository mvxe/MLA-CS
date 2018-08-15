#-------------------------------------------------
#
# Project created by QtCreator 2018-07-31T09:19:19
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MLA-CS
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0




FORMS += \
        mainwindow.ui

LIBS += \

DISTFILES += \
    icons/emblem-nowrite.svg \
    icons/emblem-ok.svg

RESOURCES += \
    icons/icon.qrc \
    icons/icon.qrc

unix:!macx: LIBS += -L$$PWD/../Vimba_2_1/VimbaCPP/DynamicLib/x86_64bit/ -lVimbaCPP -lVimbaC

INCLUDEPATH += $$PWD/../Vimba_2_1
DEPENDPATH += $$PWD/../Vimba_2_1

HEADERS += \
    MAKO/mako.h \
    XPS/xps.h \
    gui_slots_baseclass.h \
    mainwindow.h \
    mutex_containers.h \
    mutex_containers_impl.h \
    sharedvars.h \
    TCP_con.h \
    includes.h

SOURCES += \
    MAKO/mako.cpp \
    XPS/xps.cpp \
    gui_settingstab.cpp \
    gui_slots_baseclass.cpp \
    main.cpp \
    mainwindow.cpp \
    mutex_containers.cpp \
    sharedvars.cpp \
    TCP_con.cpp
