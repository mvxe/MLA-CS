#ifndef CALIB_H
#define CALIB_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
class pgFocusGUI;
class pgMoveGUI;
class pgBoundsGUI;
class pgDepthEval;
class pgBeamAnalysis;
class QSpinBox;

class pgCalib: public QWidget{
    Q_OBJECT
public:
    pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv, pgBeamAnalysis* pgBeAn);
    rtoml::vsr conf;                //configuration map

    QWidget* gui_activation;
    QWidget* gui_settings;
    QWidget* gui_processing;
    bool goToNearestFree(double radDilat, double radRandSpread); //returns true if failed (no free point nearby)
    void drawWriteArea(cv::Mat* img);
private:
    pgFocusGUI* pgFGUI;
    pgMoveGUI* pgMGUI;
    pgBoundsGUI* pgBGUI;
    pgScanGUI* pgSGUI;
    pgDepthEval* pgDpEv;
    pgBeamAnalysis* pgBeAn;

    //activation
    QVBoxLayout* alayout;
//    hidCon* hcGoToNearestFree;
//    QPushButton* btnGoToNearestFree;
//    val_selector* selRadDilGoToNearestFree;
//    val_selector* selRadSprGoToNearestFree;
    std::string saveFolderName{""};
    HQPushButton* btnWriteCalib;

    //settings
    QVBoxLayout* slayout;
    twd_selector* calibMethod;
    vtwid* calibMethodFindNearest;
        val_selector* selWriteCalibFocusDoNMeas;
        val_selector* selWriteCalibFocusReFocusNth;
        val_selector* selWriteCalibFocusRadDil;
        val_selector* selWriteCalibFocusRadSpr;
        val_selector* selWriteCalibFocusBlur;
        val_selector* selWriteCalibFocusThresh;
        val_selector* selWriteCalibFocusRange;
        val_selector* selWriteCalibFocusPulseIntensity;
        val_selector* selWriteCalibFocusPulseDuration;
    vtwid* calibMethodArray;
        val_selector* selArrayXsize;
        val_selector* selArrayYsize;
        val_selector* selArraySpacing;
        smp_selector* selArrayType;
        checkbox_gs* transposeMat;
        val_selector* selArrayDurA;
        val_selector* selArrayDurB;
        val_selector* selArrayFocA;
        val_selector* selArrayFocB;
        val_selector* selArrayOneScanN;
        checkbox_gs* selArrayRandomize;
        checkbox_gs* saveMats;
        checkbox_gs* savePic;
    vtwid* calibMethodAutoArray;
        val_selector* selAArrayDoNMes;
        val_selector* selAArrayXsize;
        val_selector* selAArrayYsize;
        val_selector* selAArraySpacing;
        val_selector* selAArrayAvgN;
        val_selector* selAArrayIntA;
        val_selector* selAArrayIntB;
        val_selector* selAArrayDurA;
        val_selector* selAArrayDurB;
        val_selector* selAArrayNGenCand;
        val_selector* selAArraySetMaskToThisHeight;
    int measCounter{0};
    bool drawWriteAreaOn{false};

    QSpinBox* cropTop;
    QSpinBox* cropBttm;
    QSpinBox* cropLeft;
    QSpinBox* cropRght;

    //processing
    QVBoxLayout* playout;
    QPushButton* btnProcessFocusMes;

    constexpr static int maxRedoScanTries=3;
    constexpr static double discardMaskRoiThresh=0.001;     //if more than 0.1% of the pixels in the roi are bad, discard and try again, up to maxRedoScanTries. If it still fails accept it anyway

    struct _pw;
    static bool _pwsort(_pw i,_pw j);

    void WCFFindNearest();
    void WCFArray();
    void WCFAArray();
    std::string makeDateTimeFolder(const std::string folder);
    void saveConf(std::string filename, double duration, double focus);
    void saveMainConf(std::string filename);
private Q_SLOTS:
//    void onGoToNearestFree();
    void onWCF();
    void onProcessFocusMes();
    void onChangeDrawWriteAreaOn(bool status);

};

#endif // CALIB_H
