#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "interthread_com.h"
#include <QString>
#include "gui_slots_baseclass.h"

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

    void on_e_xps_ip_editingFinished();
    void on_e_xps_port_editingFinished();
    void on_e_xps_xaxis_editingFinished();
    void on_e_xps_yaxis_editingFinished();
    void on_e_xps_zaxis_editingFinished();
    void on_e_xps_timeout_editingFinished();

    void on_e_rpty_timeout_editingFinished();
    void on_e_rpty_ip_editingFinished();
    void on_e_rpty_port_editingFinished();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
