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
class pgMoveGUI;
class twid;
class scanRes;
class checkbox_gs;
class QDoubleSpinBox;

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
    ~pgScanGUI();
    rtoml::vsr conf;                                //configuration map
    twid* gui_activation;
    QWidget* gui_settings;
    twid* gui_processing;
    QTimer* timer;
    QTimer* timerCM;  // we use this timer to maintain continuous measurments
    constexpr static unsigned timerCM_delay=100;

    std::atomic<bool> measurementInProgress{false}; //for outside calling functions
    void doOneRound(char cbAvg_override=0, char cbTilt_override=0, char cbRefl_override=0);
                                                    // for cbAvg_override==0, cbAvg setting is used, if cbAvg_override=1 avearage, if cbAvg_override=-1 do not average
                                                    // for cbTilt_override==0, cbTilt setting is used, if cbTilt_override=1 correct tilt, if cbTilt_override=-1 do not correct
                                                    // for cbRefl_override==0, cbRefl setting is used, if cbRefl_override=1 calc refl, if cbRefl_override=-1 do not calc refl
                                                    // this function is non blocking, check measurementInProgress to see if done
    void doNRounds(int N, double redoIfMaskHasMore=0.01, int redoN=3, cv::Rect roi={0,0,0,0}, char cbTilt_override=0, char cbRefl_override=0);
                                                    // does at least N measurements (and most N+1) with avg (cbAvg_override==1), if mask is more than redoIfMaskHasMore fraction of total(ROID) pixels, redo mesurements, up to redoN times
                                                    // this funtion is blocking, but processes qt events

    class scanRes{
    public:
        cv::Mat depth;
        cv::Mat mask;
        cv::Mat maskN;      //for convenience, much faster to do while its already on the gpu
        double min;
        double max;
        double tiltCor[2];  //X, and Y
        double pos[3];      //X,Y and Z at the time of measurement, might be useful
        double XYnmppx;     //useful if we load a scan which was done at different settings, -1 means the one in settings is valid
        int avgNum;
        cv::Mat depthSS;    //this is for calculating the standard deviation: will be empty if avgNum=1
        cv::Mat refl;       //optional: the reflectivity of the mirror
        void copyTo(scanRes& dst) const;
    };
    varShare<scanRes> result;

    mesLockProg& MLP;
    double vsConv(val_selector* vs);

    static cv::Rect lastROI;
    static void saveScan(const scanRes* scan, std::string fileName="", bool useLastROI=false, bool saveSD=true, bool saveRF=true);
    static void saveScan(const scanRes* scan, const cv::Rect &roi, std::string fileName="", bool saveSD=true, bool saveRF=true);
    static void saveScanTxt(const scanRes* scan, std::string fileName="");
    static void saveScanTxt(const scanRes* scan, const cv::Rect &roi, std::string fileName="");
    static bool loadScan(scanRes* scan, std::string fileName="");                       //return true if success
    scanRes difScans(scanRes* scan0, scanRes* scan1);
    scanRes avgScans(std::vector<scanRes>* scans, double excludeSDfactor=-1, int maxNofCorrections=10);       //does not decorrect tilt, excludeSDfactor: exclude measurements that are this many times farther from avg than SD (for example 3), negative number or 0 means no excluding
                                                                                                                    //using excludeSDfactor>0 makes the program modify scans
    static QWidget* parent; //for dialogs

    std::mutex* useCorr;                //from correction
    pgScanGUI::scanRes* cor{nullptr};
private:
    void init_gui_activation();
    void init_gui_settings();

    //activation
    QPushButton* bScan;
    QCheckBox* bScanContinuous;
    QPushButton* bCenter;
    checkbox_gs* cbCorrectTilt;
    checkbox_gs* cbAvg;
    checkbox_gs* cbGetRefl;

    //settings
    QVBoxLayout* slayout;
public:
    val_selector* led_wl;       //LED wavelength
    constexpr static int darkFrameNum=10;

    //save pixel to file
    std::mutex clickDataLock;
    bool savePix{false};
    int clickCoordX, clickCoordY;
    std::string clickFilename;

    //get max and min pixel exposure for camera exposure setting
    std::atomic<bool> getExpMinMax{false};
    pgMoveGUI* pgMGUI;

private:
    smp_selector* selectScanSetting;    //scan setting
    std::vector<scanSettings*> settingWdg;
    friend class scanSettings;
    constexpr static unsigned Nset{5};

    val_selector* coh_len;      //coherence length
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    val_selector* dis_thresh;   //if the checked peaks within peakLocRange of the freq peak are higher than peakThresh x the main peak the pixel is added to the mask (of bad pixels)
    QLabel* calcL;
    constexpr static unsigned timer_delay=500;  //if the program is busy measuring we cannot update the variables, so wait for this ammount and try again
    val_selector* exclDill;
    val_selector* tiltCorBlur;
    val_selector* tiltCorThrs;
    checkbox_gs* findBaseline;
    val_selector* findBaselineHistStep;
    smp_selector* debugDisplayModeSelect;
    val_selector* avgDiscardCriteria;
    val_selector* triggerAdditionalDelay;
    unsigned timeout{500};    // we hardcode a timeout (in ms) for waiting for frames, in case something goes wrong with trigger
    QPushButton* saveAvgMess;
    QPushButton* saveNextMirrorBaselineHist;
    std::string fnameSaveNextMirrorBaselineHist;

    unsigned totalFrameNum;
    unsigned peakLoc;           //the expected peak position in the FFT spectrum
    constexpr static unsigned peakLocRange=2;       //we check this many peaks from each side of peakLoc
    unsigned i2NLambda;         //the number of expected wavelengths x2 (ie number of expected maxima and minima)

    exec_ret PVTret;
    CTRL::CO* COmeasure{nullptr};
    double COfps;
    std::atomic<bool> CORdy{false};
    void updateCO(std::string &report);

    std::atomic<bool> keepMeasuring{false};

    double total_meas_time;

    std::atomic<bool> bSaveAvgMess{false};    // for autosaving raw data - for debug and bookeeping purposes - for saving individual measurements that are being averaged.
    std::string stringSaveAvgMess;
    int saveIter;

    QDoubleSpinBox* xDifShift;
    QDoubleSpinBox* yDifShift;

    void _doOneRound(char cbAvg_override=0, char cbTilt_override=0, char cbRefl_override=0);
    void calcExpMinMax(FQ* framequeue, cv::Mat* mask);
    void _correctTilt(scanRes* res);
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onBScan();
    void onBScanContinuous(bool status);
    void onMenuChange(int index);
    void onBSaveAvgMess();
    void onBSaveNextMirrorBaselineHist();
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
    QLabel* calcL;
    val_selector* exclDill;
    val_selector* tiltCorBlur;
    val_selector* tiltCorThrs;
    checkbox_gs* findBaseline;
    val_selector* findBaselineHistStep;
    pgScanGUI* parent;
};

#endif // SCAN_H
