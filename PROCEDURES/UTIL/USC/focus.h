#ifndef FOCUS_H
#define FOCUS_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;
class PVTobj;
class QLabel;
class pgScanGUI;

namespace cv{class Mat;}

class pgFocusGUI: public QWidget{
    Q_OBJECT
    //GUI
public:
    pgFocusGUI(std::mutex &_lock_mes, std::mutex &_lock_comp, pgScanGUI *pgSGUI);
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    std::mutex &_lock_mes;
    std::mutex &_lock_comp;
 private:
    void init_gui_activation();
    void init_gui_settings();
    pgScanGUI *pgSGUI;          //we share some settings with this

    //activation
    QHBoxLayout* alayout;


    //settings
    QVBoxLayout* slayout;
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    QLabel* calcL;

    int totalFrameNum;
    constexpr static unsigned timer_delay=500;
    PVTobj* PVTScan;
    exec_ret PVTret;
    bool PVTsRdy=false;
    void updatePVT(std::string &report);

    double refocus();
    double vsConv(val_selector* vs);
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onRefocus();
};

#endif // FOCUS_H
