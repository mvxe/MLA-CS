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
class twid;

namespace cv{class Mat;}

class pgFocusGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgFocusGUI(mesLockProg& MLP, pgScanGUI* pgSGUI);
    rtoml::vsr conf;            //configuration map
    twid* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    mesLockProg& MLP;
    void refocus();
 private:
    smp_selector* selectFocusSetting;
    std::vector<focusSettings*> settingWdg;
    friend class focusSettings;
    constexpr static unsigned Nset{3};
    void init_gui_activation();
    void init_gui_settings();
    pgScanGUI* pgSGUI;          //we share some settings with this

    //activation
    QPushButton* bFocus;

    //settings
    QVBoxLayout* slayout;
    val_selector* range;        //scan range
    val_selector* pphwl;         //points per wavelength
    QLabel* calcL;
    double displacementOneFrame;
    double readRangeDis;
    val_selector* gaussianBlur;
    QPushButton* btnSaveNextDebugFocus;

    unsigned totalFrameNum;
    constexpr static unsigned timer_delay=500;
    CTRL::CO* COmeasure{nullptr};
    double COfps;
    std::atomic<bool> CORdy{false};
    void updateCO(std::string &report);
    unsigned const timeout{500};        // we hardcode a timeout (in ms) for waiting for frames, in case something goes wrong with trigger

    double total_meas_time;
    std::string saveNextFocus{""};

    double vsConv(val_selector* vs);

public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onRefocus();
    void onMenuChange(int index);
    void onBtnSaveNextDebugFocus();
};

class focusSettings: public QWidget{
    Q_OBJECT
    //GUI
public:
    focusSettings(uint num, pgFocusGUI* parent);
    QVBoxLayout* slayout;
    val_selector* range;        //scan range
    val_selector* pphwl;         //points per wavelength
    pgFocusGUI* parent;
};

#endif // FOCUS_H
