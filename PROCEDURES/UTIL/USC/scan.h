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
class FQ;
class scanSettings;
class mesLockProg;
class colorMap;

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
    pgScanGUI(mesLockProg& MLP);
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;
    QTimer* timerCM;  // we use this timer to maintain continuous measurments
    constexpr static unsigned timerCM_delay=100;

    std::atomic<bool> measurementInProgress{false}; //for outside calling functions
    void doOneRound();

    struct scanRes{
        cv::Mat depth;
        cv::Mat mask;
        cv::Mat maskN;  //for convenience, much faster to do while its already on the gpu
        double min;
        double max;
        double tiltCor[2];  //X, and Y
        double pos[3];  //X,Y and Z at the time of measurement, might be useful
        double XYnmppx; //useful if we load a scan which was done at different settings, -1 means the one in settings is valid
    };
    varShare<scanRes> result;

    mesLockProg& MLP;
    double vsConv(val_selector* vs);
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

    //save pixel to file
    std::mutex clickDataLock;
    bool savePix{false};
    int clickCoordX, clickCoordY;
    std::string clickFilename;

    //get max and min pixel exposure for camera exposure setting
    std::atomic<bool> getExpMinMax{false};
    colorMap* cMap{nullptr};
private:
    smp_selector* selectScanSetting;    //scan setting
    std::vector<scanSettings*> settingWdg;
    constexpr static unsigned Nset{5};

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

    double total_meas_time;

    void _doOneRound();
    void calcExpMinMax(FQ* framequeue, cv::Mat* mask);
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onBScanOne();
    void onBScanContinuous(bool status);
    void setCorrectTilt(bool state){correctTilt=state;}
    void onMenuChange(int index);
Q_SIGNALS:
    void doneExpMinmax(int min, int max);
};

class scanSettings: public QWidget{
    Q_OBJECT
    //GUI
public:
    scanSettings(uint num, pgScanGUI* parent);
    QVBoxLayout* slayout;
    val_selector* led_wl;
    val_selector* coh_len;
    val_selector* range;
    val_selector* ppwl;
    val_selector* max_vel;
    val_selector* max_acc;
    val_selector* dis_thresh;
    pgScanGUI* parent;
};

#endif // SCAN_H
