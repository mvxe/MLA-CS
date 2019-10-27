#ifndef SCAN_H
#define SCAN_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class val_selector;
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
    pgScanGUI();
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;

    void doOneRound();      // this automatically does the offset in case we are not offset
    void getCentered();     // this recenters
    std::atomic<bool> roundDone{true};      //this signals whether any kind of measurements accessing the stages are done
    std::atomic<bool> procDone{true};       //this signals whether framebuffer processing is done

    cvMat_safe measuredM;   //contains the mask
    cvMat_safe measuredP;   //contains the phase map
    cvMat_safe measuredPU;  //contains the unwrapped phase map
 private:
    void init_gui_activation();
    void init_gui_settings();
    std::mutex _lock_mes;
    std::mutex _lock_comp;

    //activation
    QHBoxLayout* alayout;
    QPushButton* bScanOne;
    QPushButton* bScanContinuous;
    QPushButton* bCenter;

    //settings
    QVBoxLayout* slayout;
    val_selector* led_wl;       //LED wavelength
    val_selector* coh_len;      //coherence length
    val_selector* range;        //scan range
    val_selector* ppwl;         //points per wavelength
    val_selector* max_vel;      //maximum microscope axis velocity
    val_selector* max_acc;      //maximum microscope axis acceleration
    val_selector* dis_thresh;   //if the checked peaks within peakLocRange of the freq peak are higher than peakThresh x the main peak the pixel is added to the mask (of bad pixels)
    std::atomic<bool> changed{true};
    QLabel* calcL;
    unsigned work_call_time=100;


    const int darkFrameNum=4;
    int totalFrameNum;
    int peakLoc;        //the expected peak position in the FFT spectrum
    constexpr static int peakLocRange=2;    //we check this many peaks from each side of peakLoc
    int i2NLambda;       //the number of expected wavelengths x2 (ie number of expected maxima and minima)

    bool isOffset=false;    // false=we are centered, true=we are offset and ready to start
    double setOffset=0;
    PVTobj* PVTtoPos[2];    // [0] is downward, [1] is upward
    PVTobj* PVTmeasure[2];  // [0] is downward, [1] is upward
    exec_ret PVTret;
    bool PVTsRdy=false;
    int dir=0;              // to be used as PVT[dir] and flipped after each move (valid values: 0 and 1)
    void updatePVTs(std::string &report);   // update PVTs whenever measurement paramaters are changed, returns true if PVT fails or accels/speeds are to high

    void _doOneRound();
    double vsConv(val_selector* vs);
    std::atomic<bool> keepMeasuring{false};
private Q_SLOTS:
    void work_fun();

    void onBScanOne();
    void onBScanContinuous(bool status);
    void onBCenter();
};

#endif // SCAN_H
