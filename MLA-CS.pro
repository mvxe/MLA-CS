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


CONFIG += c++17 object_parallel_to_source           #without object_parallel_to_source you cmake doesnt diferentiate between same named files in different folders

FORMS += \
    GUI/mainwindow.ui

LIBS += \

DISTFILES += \
    icons/emblem-nowrite.svg \
    icons/emblem-ok.svg

RESOURCES += \
    icons/icon.qrc \
    icons/icon.qrc

unix:!macx: LIBS += -L$$PWD/../Vimba_2_1/VimbaCPP/DynamicLib/x86_64bit/ -lVimbaCPP -lVimbaC -lcurlpp -lcurl
#DEP: sudo apt install libcurlpp-dev libcurlpp0

INCLUDEPATH += $$PWD/../Vimba_2_1
DEPENDPATH += $$PWD/../Vimba_2_1

HEADERS += \
    DEV/MAKO/mako.h \
    DEV/XPS/xps.h \
    includes.h \
    DEV/MAKO/mako_events.h \
    DEV/MAKO/vmbwrap.h \
    DEV/MAKO/frame_queues.h \
    globals.h \
    GUI/slots_baseclass.h \
    DEV/MAKO/camobj.h \
    UTIL/utility.h \
    UTIL/containers.h \
    UTIL/containers_template.h \
    DEV/XPS/xps_template.h \
    DEV/XPS/_config.h \
    DEV/MAKO/_config.h \
    GUI/TAB_CAMERA/_config.h \
    GUI/TAB_CONNECTION/_config.h \
    GUI/TAB_SETTINGS/_config.h \
    DEV/RPTY/rpty.h \
    DEV/RPTY/_config.h \
    GUI/mainwindow.h \
    GUI/gui_includes.h \
    DEV/TCP_con.h \
    GUI/TAB_POSITIONERS/_config.h \
    PROCEDURES/procedure.h \
    PROCEDURES/UTIL/find_focus.h \
    PROCEDURES/_incl_procedures.h

SOURCES += \
    DEV/MAKO/mako.cpp \
    DEV/XPS/xps.cpp \
    main.cpp \
    DEV/MAKO/mako_events.cpp \
    DEV/MAKO/frame_queues.cpp \
    globals.cpp \
    GUI/settingstab.cpp \
    GUI/slots_baseclass.cpp \
    GUI/mainwindow.cpp \
    DEV/MAKO/camobj.cpp \
    UTIL/containers.cpp \
    GUI/TAB_CONNECTION/tab_main.cpp \
    GUI/TAB_SETTINGS/tab_main.cpp \
    GUI/TAB_CAMERA/tab_main.cpp \
    DEV/RPTY/rpty.cpp \
    DEV/TCP_con.cpp \
    GUI/TAB_POSITIONERS/tab_main.cpp \
    PROCEDURES/procedure.cpp \
    PROCEDURES/UTIL/find_focus.cpp


unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv
}
