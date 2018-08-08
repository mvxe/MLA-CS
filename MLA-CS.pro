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


SOURCES += \
        main.cpp \
        gui_settingstab.cpp \
        mainwindow.cpp \
        communication_methods.cpp \
    mutex_containers.cpp \
    gui_slots_baseclass.cpp


HEADERS += \
        mainwindow.h \
        communication_methods.h \
        mutex_containers.h \
        mutex_containers_impl.h \
        interthread_com.h \
    gui_slots_baseclass.h


FORMS += \
        mainwindow.ui

LIBS += \
       -lboost_system\

