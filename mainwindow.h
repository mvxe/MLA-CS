#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "includes.h"
#include "gui_slots_baseclass.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow , public GUI_slots_baseclass
{
    Q_OBJECT

public:
    explicit MainWindow(QApplication* qapp, QWidget *parent = 0);
    ~MainWindow();
    static void change_xps_z(int value);
private slots:
    void sync_settings();
    void GUI_update();
    void updateCamMenu();
    void program_exit();

    void on_e_xps_ip_editingFinished();
    void on_e_xps_port_editingFinished();
    void on_e_xps_xaxis_editingFinished();
    void on_e_xps_yaxis_editingFinished();
    void on_e_xps_zaxis_editingFinished();
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

    void on_dial_valueChanged(int value);


private:
    QApplication* qapp;
    Ui::MainWindow *ui;
    QMenu *menu;
    std::vector<QAction*> actptrs;
    QTimer *timer;

    QPixmap* px_online;
    QPixmap* px_offline;

    int dialval;
};

class mtlabel : public QLabel{
    using QLabel::QLabel;
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
};



#endif // MAINWINDOW_H
