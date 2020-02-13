#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
namespace cv{class Mat;}
class pgMoveGUI;
class FQ;
class pgScanGUI;
class mesLockProg;
class cameraSett;

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis(mesLockProg& MLP, pgMoveGUI* pgMGUI, pgScanGUI* pgSGUI);
    QWidget* gui_settings;
    QWidget* gui_activation;
//    bool getCalibWritingBeam(float* r=nullptr, float* dx=nullptr, float* dy=nullptr, bool correct=true);                        //return true on failure (also wont correct if faliure)
    void getCalibWritingBeamRange(double *rMinLoc, double *xMin, double *yMin, int frames, double range, bool flipDir=false);   //flipDir makes the scan to scan from the other direction, if you dont need rMinLoc, xMin or yMin you can pass nullptr
    const double& writeBeamCenterOfsX{_writeBeamCenterOfsX};
    const double& writeBeamCenterOfsY{_writeBeamCenterOfsY};
    const double* extraFocusOffsetVal;
public Q_SLOTS:
    bool correctWritingBeamFocus(bool reCenter=true);   //does getCalibWritingBeamRange twice/thrice, using the parameters in settings. Corrects focus. reCenter also corrects the center, return 0 on success
private:
    mesLockProg& MLP;
    pgMoveGUI* pgMGUI;
    pgScanGUI* pgSGUI;
    double _writeBeamCenterOfsX;    //the center offset in pixels
    double _writeBeamCenterOfsY;
    cc_save<double> saveWBCX{_writeBeamCenterOfsX, 0,&go.pos_config.save,"pgBeamAnalysis_saveWBCX"};
    cc_save<double> saveWBCY{_writeBeamCenterOfsY, 0,&go.pos_config.save,"pgBeamAnalysis_saveWBCY"};

    QVBoxLayout* slayout;
    QPushButton* btnReset;
//    twds_selector* method_selector;
//        QWidget* methodSimple;
//        QVBoxLayout* methodSimpleLayout;
//            val_selector* selThresh;
//            val_selector* avgNum;
//        QWidget* methodEllipsse;
//        QVBoxLayout* methodEllipsseLayout;
//            val_selector* selMaxRoundnessDev;
//            val_selector* selCannyThreshL;
//            val_selector* selCannyThreshU;
//            val_selector* selMinPixNum;
//    QPushButton* btnSaveNextDebug;
    val_selector* extraOffsX;
    val_selector* extraOffsY;
//    val_selector* cameraExposure;

    //  Get beam focus
    val_selector* wideFrames;
    val_selector* wideRange;
    val_selector* accuFrames;
    val_selector* accuRange;
    val_selector* selThresh;
    checkbox_save* doExtraFocusMesDifDir;
    QPushButton* btnSaveNextDebugFocus;
    val_selector* extraFocusOffset;
    QPushButton* exOfsCalibBtn;


    QVBoxLayout* alayout;
//    QPushButton* btnGetCenter;
    QPushButton* btnGetCenterFocus;
    double X_start, Y_start;

    struct spot{
        float x,y,r;
        float dx,dy,dd;
    };
    std::mutex spotLock;    //for multithreading solve
    void solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone);
    static bool sortSpot(spot i,spot j);
//    std::string saveNext{""};                           //these arent thread safe, ikr, but unlikely to cause problems
    std::string saveNextFocus{""}; int numSave;

    void ctrlRedLaser(bool state);
//    void armRedLaser();
    bool waitTillLEDIsOff(FQ* framequeueDisp); //return 0 on sucess
private Q_SLOTS:
//    void getWritingBeamCenter();
//    void onBtnSaveNextDebug();
    void onBtnSaveNextDebugFocus();
    void onBtnReset();
    void onExOfsCalibBtn(bool state);
};

#endif // BEAMANL_H
