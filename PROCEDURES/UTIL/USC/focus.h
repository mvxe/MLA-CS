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

namespace cv{class Mat;}

class pgFocusGUI: public QWidget{
    Q_OBJECT
    //GUI
public:
    pgFocusGUI(std::mutex& _lock_mes, std::mutex& _lock_comp, pgScanGUI* pgSGUI, pgTiltGUI* pgTGUI);
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    std::mutex& _lock_mes;
    std::mutex& _lock_comp;
 private:
    void init_gui_activation();
    void init_gui_settings();
    pgScanGUI* pgSGUI;          //we share some settings with this
    pgTiltGUI* pgTGUI;          //we call its functions to tilt

    //activation
    QHBoxLayout* alayout;
    QPushButton* bFocus;

    //settings
    QVBoxLayout* slayout;
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    val_selector* tilt;         //ammount of tilt
    QPushButton* testTilt;
    QLabel* calcL;
    double mmPerFrame;

    int totalFrameNum;
    constexpr static unsigned timer_delay=500;
    PVTobj* PVTScan;
    exec_ret PVTret;
    bool PVTsRdy=false;
    void updatePVT(std::string &report);

    void refocus();
    void _refocus();
    double vsConv(val_selector* vs);
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onRefocus();
    void onTestTilt(bool state);
};

#endif // FOCUS_H
