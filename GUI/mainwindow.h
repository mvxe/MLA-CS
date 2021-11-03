#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "GUI/gui_includes.h"
#include "GUI/slots_baseclass.h"
#include "GUI/TAB_CAMERA/tab_camera.h"
#include "GUI/TAB_DEVICES/tab_devices.h"
#include "GUI/TAB_SETTINGS/tab_settings.h"
#include "GUI/TAB_POSITIONERS/tab_positioners.h"
#include "GUI/tab_monitor.h"
class FQ;

namespace Ui {
class MainWindow;
class tab_settings;
class tab_connection;
class tab_devices;
}

class MainWindow : public QMainWindow , public GUI_slots_baseclass, public tab_settings, public tab_positioners
{
    Q_OBJECT
public:
    explicit MainWindow(QApplication* qapp, QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void sync_settings();
    void GUI_update();
    void updateCamMenu(QMenu* menuN);
    void updateCncMenu(QMenu* menuN);
    void program_exit();

    void on_e_rpty_timeout_editingFinished();
    void on_e_rpty_ip_editingFinished();
    void on_e_rpty_port_editingFinished();

    void on_tabWidget_currentChanged(int index);
    void on_cam1_select_triggered(QAction *arg1);
    void cam1_select_show();

    void cam2_select_show();
    void on_cam2_select_triggered(QAction *arg1);

    void cnc_select_show();
    void on_cnc_select_triggered(QAction *arg1);

    void on_sl_util_expo_valueChanged(int value);



    void on_pushButton_2_released();

    void on_pushButton_released();

    void on_doubleSpinBox_2_editingFinished();

    void on_pushButton_3_released();

    void on_pushButton_4_released();

    void on_pushButton_6_released();

    void on_pushButton_5_released();

    void on_pushButton_7_released();

    void on_pushButton_8_released();

    void on_doubleSpinBox_5_valueChanged(double arg1);

    void on_pushButton_11_clicked();

    void on_pushButton_13_released();

    void on_rpty_rst_btn_released();

private:
    QApplication* qapp;
    Ui::MainWindow *ui;
    QMenu *menu;
    QMenu *menu2;
    QMenu *menu3;
    QTimer *timer;
    QShortcut *shortcut;

    QPixmap px_online{":/emblem-ok.svg"};
    QPixmap px_offline{":/emblem-nowrite.svg"};

    int dialval{0};
    bool iuScope_con{false};
    bool utilCam_con{false};
    bool rpty_con{false};
    bool cnc_con{false};

    FQ* iuScope_img;    //the framequeue for display
    FQ* utilCam_img;

    double a{0},b{0}; //TODO remove this dirty fix

    tab_devices* tabDev;
    tab_monitor* tabMon;
    tab_camera* tabCam;

    /* beam profiler variables, TODO move this elsewhere*/
    std::vector<cv::Mat>* mats{nullptr};
    int expsize{0};
    std::mutex matlk;
    int fitdiv=64;
    bool matsbar{true};
    double pixdim;      //in um
    double stepsize;    //in mm
    struct fitar{
        double posX[2];         // [0] is var, [1] is error
        double posY[2];
        double wdtX[2];
        double wdtY[2];
        double pow[2];
        double bgn[2];
        double ang[2];
    };
    std::vector<fitar> fitres;
    const char keyword[50]="iwqnxmcaiofa";

    int lastIndex=0;

    bool cleanedTabs=false;

    /* up to here*/
};

#endif // MAINWINDOW_H
