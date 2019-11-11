#ifndef SCAN_H
#define SCAN_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class val_selector;
class smp_selector;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class PVTobj;
class QPushButton;
class QCheckBox;

class pScan: public sproc{
public:
    pScan();
    ~pScan();
private:
    void run();
};


class pgScanGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgScanGUI(std::mutex& _lock_mes, std::mutex& _lock_comp);
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;
    QTimer* timerCM;  // we use this timer to maintain continuous measurments. If a measurement thread does this recursively opencv/opencl for some reason shits itself.
    constexpr static unsigned timerCM_delay=100;

    std::atomic<bool> measurementInProgress{false}; //for outside calling functions
    void doOneRound();

    cvMat_safe scanRes;     //contains the scan result
    cvMat_safe mask;        //contains the excluded pixel mask
    cvMat_safe maskN;       //inverse mask

    std::mutex& _lock_mes;
    std::mutex& _lock_comp;
    double vsConv(val_selector* vs);

    std::atomic<double> phiXres;
    std::atomic<double> phiYres;
private:
    void init_gui_activation();
    void init_gui_settings();

    //activation
    QHBoxLayout* alayout;
    QPushButton* bScanOne;
    QPushButton* bScanContinuous;
    QPushButton* bCenter;
    QCheckBox* cbCorrectTilt;
    bool correctTilt;
    cc_save<bool> sv_correctTilt{correctTilt,false,&go.gui_config.save,"pgScanGUI_ct"};

    //settings
    QVBoxLayout* slayout;
public:
    val_selector* led_wl;       //LED wavelength
    val_selector* max_vel;      //maximum microscope axis velocity
    val_selector* max_acc;      //maximum microscope axis acceleration
    constexpr static int darkFrameNum=4;
private:
    val_selector* coh_len;      //coherence length
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    val_selector* dis_thresh;   //if the checked peaks within peakLocRange of the freq peak are higher than peakThresh x the main peak the pixel is added to the mask (of bad pixels)
    QLabel* calcL;
    constexpr static unsigned timer_delay=500;  //if the program is busy measuring we cannot update the variables, so wait for this ammount and try again
    smp_selector* debugDisplayModeSelect;

    int totalFrameNum;
    int peakLoc;        //the expected peak position in the FFT spectrum
    constexpr static int peakLocRange=2;    //we check this many peaks from each side of peakLoc
    int i2NLambda;       //the number of expected wavelengths x2 (ie number of expected maxima and minima)

    PVTobj* PVTmeasure;
    exec_ret PVTret;
    std::atomic<bool> PVTsRdy{false};
    void updatePVTs(std::string &report);   // update PVTs whenever measurement paramaters are changed, returns true if PVT fails or accels/speeds are to high

    std::atomic<bool> keepMeasuring{false};
    void _doOneRound();
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onBScanOne();
    void onBScanContinuous(bool status);
    void setCorrectTilt(bool state){correctTilt=state;}
};

#endif // SCAN_H
