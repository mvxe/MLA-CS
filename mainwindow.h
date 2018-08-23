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

private:
    QApplication* qapp;
    Ui::MainWindow *ui;
    QMenu *menu;
    std::vector<QAction*> actptrs;
    QTimer *timer;

    QPixmap* px_online;
    QPixmap* px_offline;
};

#endif // MAINWINDOW_H
