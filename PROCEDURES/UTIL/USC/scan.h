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

class pScan: public sproc
{
public:
    pScan();
    ~pScan();
private:
    void run();
};


class pgScanGUI: public QWidget{
    Q_OBJECT
    //GUI
public:
    pgScanGUI(std::mutex& _lock_mes, std::mutex& _lock_comp);
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    void doOneRound();

    cvMat_safe scanRes;     //contains the scan result
    cvMat_safe mask;        //contains the excluded pixel mask

    std::mutex& _lock_mes;
    std::mutex& _lock_comp;
    double vsConv(val_selector* vs);
private:
    void init_gui_activation();
    void init_gui_settings();

    //activation
    QHBoxLayout* alayout;
    QPushButton* bScanOne;
    QPushButton* bScanContinuous;
    QPushButton* bCenter;

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

    void _doOneRound();
    std::atomic<bool> keepMeasuring{false};
    cv::UMat* resultFinalPhase{nullptr};
    cv::UMat Umat2D;
    cv::UMat Ufft2D;
    cv::UMat magn;
    std::vector<cv::UMat> planes{2};
    cv::UMat cmpRes;
    cv::UMat cmpFinRes;
public Q_SLOTS:
    void recalculate();
private Q_SLOTS:
    void onBScanOne();
    void onBScanContinuous(bool status);
};

#endif // SCAN_H
