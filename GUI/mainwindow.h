#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "GUI/gui_includes.h"
#include "GUI/slots_baseclass.h"
#include "GUI/TAB_CAMERA/tab_camera.h"
#include "GUI/TAB_DEVICES/tab_devices.h"
#include "GUI/TAB_SETTINGS/tab_settings.h"
#include "GUI/TAB_POSITIONERS/tab_positioners.h"
#include "GUI/tab_temp_plot.h"
class FQ;

namespace Ui {
class MainWindow;
class tab_camera;
class tab_settings;
class tab_connection;
class tab_devices;
}

class MainWindow : public QMainWindow , public GUI_slots_baseclass, public tab_camera, public tab_settings, public tab_positioners
{
    Q_OBJECT
public:
    explicit MainWindow(QApplication* qapp, QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void sync_settings();
    void GUI_update();
    void updateCamMenu();
    void program_exit();

    void on_e_xps_ip_editingFinished();
    void on_e_xps_port_editingFinished();
    void on_e_xps_xyz_editingFinished();
    void on_e_xps_timeout_editingFinished();

    void on_e_rpty_timeout_editingFinished();
    void on_e_rpty_ip_editingFinished();
    void on_e_rpty_port_editingFinished();

    void on_tabWidget_currentChanged(int index);
    void on_cam1_select_triggered(QAction *arg1);
    void cam1_select_show();
    void on_sl_xsens_valueChanged(int value);
    void on_sl_ysens_valueChanged(int value);
    void on_sl_zsens_valueChanged(int value);
    void on_sl_expo_valueChanged(int value);
    void on_btm_kill_released();
    void on_btn_home_released();
    void on_btn_X_dec_released();
    void on_btn_Y_dec_released();
    void on_btn_Z_dec_released();
    void on_btn_X_min_released();
    void on_btn_Y_min_released();
    void on_btn_Z_min_released();
    void on_btn_X_max_released();
    void on_btn_Y_max_released();
    void on_btn_Z_max_released();
    void on_btn_X_inc_released();
    void on_btn_Y_inc_released();
    void on_btn_Z_inc_released();

    void on_btn_focus_released();

    void on_btn_depthdmap_released();

    void on_btn_calXY_released();

    void on_btn_wrtingTest_released();

    void on_btn_save_img_released();

    void on_btn_PBurnArray_released();

private:
    QApplication* qapp;
    Ui::MainWindow *ui;
    QMenu *menu;
    std::vector<QAction*> actptrs;
    QTimer *timer;
    QShortcut *shortcut;

    QPixmap px_online{":/emblem-ok.svg"};
    QPixmap px_offline{":/emblem-nowrite.svg"};

    int dialval{0};
    bool xps_con{false};
    bool iuScope_con{false};
    bool rpty_con{false};

    FQ* iuScope_img;    //the framequeue for display

    double a{0},b{0}; //TODO remove this dirty fix

    tab_devices* tabDev;
    tab_temp_plot* tabPlot;
    int donth=0;
};

class mtlabel : public QLabel{
    using QLabel::QLabel;
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
public:
    MainWindow* pmw;
};



#endif // MAINWINDOW_H
