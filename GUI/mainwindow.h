#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "GUI/gui_includes.h"
#include "GUI/slots_baseclass.h"
#include "GUI/TAB_CAMERA/tab_camera.h"
#include "GUI/TAB_DEVICES/tab_devices.h"
class FQ;

namespace Ui {
class MainWindow;
class tab_settings;
class tab_connection;
class tab_devices;
}

class MainWindow : public QMainWindow , public GUI_slots_baseclass
{
    Q_OBJECT
public:
    explicit MainWindow(QApplication* qapp, QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void program_exit();

    void on_tabWidget_currentChanged(int index);

private:
    QApplication* qapp;
    Ui::MainWindow *ui;
    QMenu *menu;
    QShortcut *shortcut;

    tab_devices* tabDev;
    tab_camera* tabCam;


    int lastIndex=0;

    bool cleanedTabs=false;

    /* up to here*/
};

#endif // MAINWINDOW_H
