#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
namespace cv{class Mat;}
class pgMoveGUI;
class FQ;
class pgScanGUI;
class mesLockProg;

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis(mesLockProg& MLP, pgMoveGUI* pgMGUI, pgScanGUI* pgSGUI);
    QWidget* gui_settings;
    QWidget* gui_activation;
    bool getCalibWritingBeam(float* r, float* dx=nullptr, float* dy=nullptr, bool correct=true);   //return true on failure (also wont correct if faliure)
    void getCalibWritingBeamRange(double *rMinLoc, int frames, double range, bool flipDir=0);      //flipDir makes the scan to scan from the other direction
    const float& writeBeamCenterOfsX{_writeBeamCenterOfsX};
    const float& writeBeamCenterOfsY{_writeBeamCenterOfsY};
private:
    mesLockProg& MLP;
    pgMoveGUI* pgMGUI;
    pgScanGUI* pgSGUI;
    float _writeBeamCenterOfsX;        //the center offset in pixels
    float _writeBeamCenterOfsY;
    cc_save<float> saveWBCX{_writeBeamCenterOfsX, 0,&go.pos_config.save,"pgBeamAnalysis_saveWBCX"};
    cc_save<float> saveWBCY{_writeBeamCenterOfsY, 0,&go.pos_config.save,"pgBeamAnalysis_saveWBCY"};

    QVBoxLayout* slayout;
    QPushButton* btnReset;
    twds_selector* method_selector;
        QWidget* methodSimple;
        QVBoxLayout* methodSimpleLayout;
            val_selector* selThresh;
            val_selector* avgNum;
        QWidget* methodEllipsse;
        QVBoxLayout* methodEllipsseLayout;
            val_selector* selMaxRoundnessDev;
            val_selector* selCannyThreshL;
            val_selector* selCannyThreshU;
            val_selector* selMinPixNum;
    QPushButton* btnSaveNextDebug;
    val_selector* extraOffsX;
    val_selector* extraOffsY;
//    val_selector* cameraExposure;

    //  Get beam focus
    val_selector* wideFrames;
    val_selector* wideRange;
    val_selector* accuFrames;
    val_selector* accuRange;
    QPushButton* btnSaveNextDebugFocus;
    val_selector* extraFocusOffset;


    QVBoxLayout* alayout;
    QPushButton* btnGetCenter;
    QPushButton* btnGetCenterFocus;

    struct spot{
        float x,y,r;
        float dx,dy,dd;
    };
    std::mutex spotLock;    //for multithreading solve
    void solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone);
    static bool sortSpot(spot i,spot j);
    std::string saveNext{""};                           //these arent thread safe, ikr, but unlikely to cause problems
    std::string saveNextFocus{""}; int numSave;

    bool turnOnRedLaserAndLEDOff(FQ* framequeueDisp); //return 0 on sucess
private Q_SLOTS:
    void getWritingBeamCenter();
    void getWritingBeamFocus();
    void onBtnSaveNextDebug();
    void onBtnSaveNextDebugFocus();
    void onBtnReset();
};

#endif // BEAMANL_H
