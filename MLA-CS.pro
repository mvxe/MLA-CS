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

DEFINES += QT_DEPRECATED_WARNINGS CVPLOT_HEADER_ONLY=true

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CC = gcc-10
QMAKE_CXX = g++-10
CONFIG += object_parallel_to_source no_keywords           #without object_parallel_to_source you cmake doesnt diferentiate between same named files in different folders, no_keywords solves a conflict with glibc
QMAKE_CXXFLAGS += -std=c++20

FORMS += \
    GUI/mainwindow.ui

LIBS += \

DISTFILES +=

RESOURCES += \
    icons/icon.qrc \
    icons/icon.qrc

unix:!macx: LIBS += -DVIENNACL_WITH_OPENCL  -lOpenCL -lgsl
#QMAKE_CXXFLAGS += -g -gdwarf        # for debugging purposes (using heaptrack)
#QMAKE_CFLAGS+="-fsanitize=address -fno-omit-frame-pointer"       # for debugging purposes : asan
#QMAKE_LFLAGS+="-fsanitize=address"

INCLUDEPATH += /usr/include/glib-2.0 \
        /usr/include/aravis-0.8 \
        .CvPlot/CvPlot/inc/

QT += charts

HEADERS += \
    DEV/RPTY/motionDevices/pinexactstage.h \
    DEV/RPTY/motionDevices/simpleservo.h \
    DEV/RPTY/rpbbserial.h \
    DEV/RPTY/rpmotion.h \
    DEV/controller.h \
    DEV/device_includes.h \
    GUI/cvplotauxobj.h \
    GUI/cvplotqwindow.h \
    UTIL/.rtoml/rtoml.hpp \
    UTIL/motioncalc.h \
    includes.h \
    globals.h \
    GUI/slots_baseclass.h \
    UTIL/utility.h \
    UTIL/containers.h \
    UTIL/containers_template.h \
    DEV/RPTY/rpty.h \
    GUI/mainwindow.h \
    GUI/gui_includes.h \
    DEV/TCP_con.h \
    PROCEDURES/procedure.h \
    PROCEDURES/UTIL/find_focus.h \
    PROCEDURES/_incl_procedures.h \
    PROCEDURES/UTIL/get_depth_map.h \
    PROCEDURES/UTIL/calibrate_xy.h \
    PROCEDURES/UTIL/writing_test.h \
    GUI/TAB_CAMERA/tab_camera.h \
    GUI/TAB_DEVICES/tab_devices.h \
    DEV/devices.h \
    PROCEDURES/UTIL/burn_array.h \
    DEV/device_flags.h \
    DEV/GCAM/_config.h \
    DEV/GCAM/arvwrap.h \
    DEV/GCAM/camobj.h \
    DEV/GCAM/frame_queues.h \
    DEV/GCAM/gcam.h \
    UTIL/pipe.h \
    DEV/RPTY/fpga_const.h \
    GUI/gui_aux_objects.h \
    UTIL/img_util.h \
    PROCEDURES/UTIL/USC/scan.h \
    PROCEDURES/UTIL/USC/tilt.h \
    PROCEDURES/UTIL/USC/move.h \
    PROCEDURES/UTIL/USC/focus.h \
    PROCEDURES/UTIL/USC/position_report.h \
    PROCEDURES/UTIL/USC/histogram.h \
    UTIL/threadpool.h \
    GUI/TAB_CAMERA/colormap.h \
    GUI/TAB_CAMERA/other_settings.h \
    UTIL/measurement.h \
    PROCEDURES/UTIL/WRT/calib.h \
    PROCEDURES/UTIL/WRT/beamanl.h \
    GUI/TAB_CAMERA/gnuplot.h \
    UTIL/pipe_template.h \
    PROCEDURES/UTIL/WRT/write.h \
    PROCEDURES/UTIL/USC/correction.h

SOURCES += \
    DEV/RPTY/motionDevices/pinexactstage.cpp \
    DEV/RPTY/motionDevices/simpleservo.cpp \
    DEV/RPTY/rpbbserial.cpp \
    DEV/RPTY/rpdevices.cpp \
    DEV/RPTY/rpmotion.cpp \
    GUI/cvplotauxobj.cpp \
    GUI/cvplotqwindow.cpp \
    main.cpp \
    globals.cpp \
    GUI/slots_baseclass.cpp \
    GUI/mainwindow.cpp \
    UTIL/containers.cpp \
    DEV/RPTY/rpty.cpp \
    DEV/TCP_con.cpp \
    PROCEDURES/procedure.cpp \
    PROCEDURES/UTIL/find_focus.cpp \
    PROCEDURES/UTIL/get_depth_map.cpp \
    PROCEDURES/UTIL/calibrate_xy.cpp \
    PROCEDURES/UTIL/writing_test.cpp \
    GUI/TAB_CAMERA/tab_camera.cpp \
    GUI/TAB_DEVICES/tab_devices.cpp \
    DEV/devices.cpp \
    PROCEDURES/UTIL/burn_array.cpp \
    DEV/GCAM/camobj.cpp \
    DEV/GCAM/frame_queues.cpp \
    DEV/GCAM/gcam.cpp \
    UTIL/pipe.cpp \
    GUI/gui_aux_objects.cpp \
    UTIL/img_util.cpp \
    PROCEDURES/UTIL/USC/scan.cpp \
    PROCEDURES/UTIL/USC/tilt.cpp \
    PROCEDURES/UTIL/USC/move.cpp \
    PROCEDURES/UTIL/USC/focus.cpp \
    PROCEDURES/UTIL/USC/position_report.cpp \
    PROCEDURES/UTIL/USC/histogram.cpp \
    UTIL/threadpool.cpp \
    GUI/TAB_CAMERA/colormap.cpp \
    GUI/TAB_CAMERA/other_settings.cpp \
    PROCEDURES/UTIL/WRT/calib.cpp \
    PROCEDURES/UTIL/WRT/beamanl.cpp \
    GUI/TAB_CAMERA/gnuplot.cpp \
    PROCEDURES/UTIL/WRT/write.cpp \
    PROCEDURES/UTIL/USC/correction.cpp


unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 glib-2.0 aravis-0.8
}
