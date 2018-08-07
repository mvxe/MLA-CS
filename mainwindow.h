#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "interthread_com.h"
#include <QString>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_e_xps_ip_editingFinished();

    void on_e_xps_port_editingFinished();

    void on_e_xps_xaxis_editingFinished();

    void on_e_xps_yaxis_editingFinished();

    void on_e_xps_zaxis_editingFinished();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
