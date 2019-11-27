#ifndef FOCUS_H
#define FOCUS_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class PVTobj;
class QLabel;
class pgScanGUI;
class pgTiltGUI;
class QPushButton;
class mesLockProg;
class smp_selector;
class focusSettings;

namespace cv{class Mat;}

class pgFocusGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgFocusGUI(mesLockProg& MLP, pgScanGUI* pgSGUI);
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    mesLockProg& MLP;
    void refocus();
    const std::atomic<bool>& focusingDone{_focusingDone};
 private:
    smp_selector* selectFocusSetting;
    std::vector<focusSettings*> settingWdg;
    constexpr static unsigned Nset{3};
    void init_gui_activation();
    void init_gui_settings();
    pgScanGUI* pgSGUI;          //we share some settings with this

    //activation
    QHBoxLayout* alayout;
    QPushButton* bFocus;

    //settings
    QVBoxLayout* slayout;
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    QLabel* calcL;
    double mmPerFrame;

    int totalFrameNum;
    constexpr static unsigned timer_delay=500;
    PVTobj* PVTScan;
    exec_ret PVTret;
    bool PVTsRdy=false;
    void updatePVT(std::string &report);

    double total_meas_time;

    void _refocus();
    std::atomic<bool> _focusingDone{false};
    double vsConv(val_selector* vs);
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onRefocus();
    void onMenuChange(int index);
};

class focusSettings: public QWidget{
    Q_OBJECT
    //GUI
public:
    focusSettings(uint num, pgFocusGUI* parent);
    QVBoxLayout* slayout;
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    pgFocusGUI* parent;
};

#endif // FOCUS_H
