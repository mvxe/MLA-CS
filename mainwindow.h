#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "icc_gui.h"
#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
