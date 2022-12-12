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

class MainWindow : public QMainWindow , public GUI_slots_baseclass{
    Q_OBJECT
public:
    explicit MainWindow(QApplication* qapp, std::vector<rtoml::vsr*>* confList);
    ~MainWindow();

private Q_SLOTS:
    void program_exit();
    void onSaveBtn();
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
};

class hQCheckBox: public QCheckBox{
    Q_OBJECT
    using QCheckBox::QCheckBox;
private Q_SLOTS:
    void parent_changed(int state);
    void child_changed(int state);
};

class varCheckList{
public:
    ~varCheckList();
    std::queue<varCheckList> queue;
    hQCheckBox* chbox;
    rtoml::vsr* conf;
};

class saveDialog : public QDialog{
    Q_OBJECT
public:
    saveDialog(QWidget* parent, std::vector<rtoml::vsr*>& confList);
private:
    QScrollArea qsa;
    QVBoxLayout layout;
    std::queue<varCheckList> vcl;
    void recursiveList(uint depth, varCheckList* vcl);
    void recursiveSave(varCheckList* vcl);
private Q_SLOTS:
    void on_finished(int result);
};

#endif // MAINWINDOW_H
