#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "sharedvars.h"
#include <QString>
#include <QTimer>
#include "gui_slots_baseclass.h"
#include <thread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow , public GUI_slots_baseclass
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void sync_settings();
    void GUI_update();

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

private:
    Ui::MainWindow *ui;

    QPixmap* px_online;
    QPixmap* px_offline;
};

#endif // MAINWINDOW_H
