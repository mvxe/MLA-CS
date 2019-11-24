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


CONFIG += c++17 object_parallel_to_source no_keywords           #without object_parallel_to_source you cmake doesnt diferentiate between same named files in different folders, no_keywords solves a conflict with glibc

FORMS += \
    GUI/mainwindow.ui

LIBS += \

DISTFILES +=

RESOURCES += \
    icons/icon.qrc \
    icons/icon.qrc

unix:!macx: LIBS += -lcurlpp -lcurl -lserial -DVIENNACL_WITH_OPENCL  -lOpenCL -ldlib -lblas -llapack

INCLUDEPATH += /usr/include/glib-2.0 \
        /usr/include/aravis-0.6

QT += charts

HEADERS += \
    DEV/XPS/xps.h \
    includes.h \
    globals.h \
    GUI/slots_baseclass.h \
    UTIL/utility.h \
    UTIL/containers.h \
    UTIL/containers_template.h \
    DEV/XPS/xps_template.h \
    DEV/XPS/_config.h \
    DEV/RPTY/rpty.h \
    DEV/RPTY/_config.h \
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
    GUI/TAB_POSITIONERS/tab_positioners.h \
    GUI/TAB_SETTINGS/tab_settings.h \
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
    GUI/tab_temp_plot.h \
    DEV/CNC/cnc.h \
    DEV/CNC/cnc_template.h \
    PROCEDURES/UTIL/profile_beam.h \
    DEV/RPTY/fpga_const.h \
    GUI/tab_monitor.h \
    DEV/ALP/alp.h \
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
    PROCEDURES/UTIL/WRT/bounds.h \
    PROCEDURES/UTIL/WRT/deptheval.h

SOURCES += \
    DEV/XPS/xps.cpp \
    main.cpp \
    globals.cpp \
    GUI/settingstab.cpp \
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
    GUI/TAB_POSITIONERS/tab_positioners.cpp \
    GUI/TAB_SETTINGS/tab_settings.cpp \
    GUI/TAB_DEVICES/tab_devices.cpp \
    DEV/devices.cpp \
    PROCEDURES/UTIL/burn_array.cpp \
    DEV/GCAM/camobj.cpp \
    DEV/GCAM/frame_queues.cpp \
    DEV/GCAM/gcam.cpp \
    UTIL/pipe.cpp \
    GUI/tab_temp_plot.cpp \
    DEV/CNC/cnc.cpp \
    PROCEDURES/UTIL/profile_beam.cpp \
    GUI/tab_monitor.cpp \
    DEV/ALP/alp.cpp \
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
    PROCEDURES/UTIL/WRT/bounds.cpp \
    PROCEDURES/UTIL/WRT/deptheval.cpp


unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 glib-2.0 aravis-0.6
}
