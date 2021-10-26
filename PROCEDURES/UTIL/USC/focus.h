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
class procLockProg;
class smp_selector;
class focusSettings;
class twid;
class pgMoveGUI;

namespace cv{class Mat;}

class pgFocusGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgFocusGUI(procLockProg& MLP, cv::Rect& sROI, pgScanGUI* pgSGUI, pgMoveGUI* pgMGUI);
    rtoml::vsr conf;            //configuration map
    twid* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    procLockProg& MLP;
    void doRefocus(bool block=true, cv::Rect ROI={0,0,0,0});

    std::atomic<bool> focusInProgress{false};   //for outside calling functions
    cv::Rect& sROI;
 private:
    void refocus(cv::Rect ROI={0,0,0,0});
    smp_selector* selectFocusSetting;
    std::vector<focusSettings*> settingWdg;
    friend class focusSettings;
    constexpr static unsigned Nset{3};
    void init_gui_activation();
    void init_gui_settings();
    pgScanGUI* pgSGUI;          //we share some settings with this
    pgMoveGUI* pgMGUI;

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
    void slotQMessageBoxWarning(QString title, QString text);
Q_SIGNALS:
    void signalQMessageBoxWarning(QString title, QString text);
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
