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

class pgCalib: public QWidget{
    Q_OBJECT
public:
    pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv, pgBeamAnalysis* pgBeAn);
    ~pgCalib();

    QWidget* gui_activation;
    QWidget* gui_settings;
    QWidget* gui_processing;
    bool goToNearestFree(double radDilat, double radRandSpread); //returns true if failed (no free point nearby)
private:
    pgFocusGUI* pgFGUI;
    pgMoveGUI* pgMGUI;
    pgBoundsGUI* pgBGUI;
    pgScanGUI* pgSGUI;
    pgDepthEval* pgDpEv;
    pgBeamAnalysis* pgBeAn;
    varShareClient<pgScanGUI::scanRes>* scanRes;

    //activation
    QVBoxLayout* alayout;
    hidCon* hcGoToNearestFree;
    QPushButton* btnGoToNearestFree;
    val_selector* selRadDilGoToNearestFree;
    val_selector* selRadSprGoToNearestFree;
    QPushButton* btnSelectSaveFolder;
    std::string saveFolderName{""};
    QPushButton* btnWriteCalibFocus;

    //settings
    QVBoxLayout* slayout;
    twds_selector* calibMethod;
    vtwid* calibMethodFindNearest;
        val_selector* selWriteCalibFocusDoNMeas;
        val_selector* selWriteCalibFocusReFocusNth;
        val_selector* selWriteCalibFocusRadDil;
        val_selector* selWriteCalibFocusRadSpr;
        val_selector* selWriteCalibFocusBlur;
        val_selector* selWriteCalibFocusThresh;
        val_selector* selWriteCalibFocusRange;
    //    checkbox_save* selWriteCalibFocusMoveOOTW;
    //    val_selector* selWriteCalibFocusMoveOOTWDis;
        val_selector* selWriteCalibFocusPulseIntensity;
        val_selector* selWriteCalibFocusPulseDuration;
    vtwid* calibMethodArray;
        val_selector* selArrayXsize;
        val_selector* selArrayYsize;
        val_selector* selArraySpacing;
        smp_selector* selArrayType;
        val_selector* selArrayIntA;
        val_selector* selArrayIntB;
        val_selector* selArrayDurA;
        val_selector* selArrayDurB;
        val_selector* selArrayFocA;
        val_selector* selArrayFocB;
        smp_selector* selArrayScanType;     //one scan, multi scan
        val_selector* selArrayOneScanN;
        checkbox_save* selArrayRandomize;
        checkbox_save* showLimits;
    int measCounter{0};

    //processing
    QVBoxLayout* playout;
    QPushButton* btnProcessFocusMes;

    void WCFFindNearest();
    void WCFArray();
private Q_SLOTS:
    void onGoToNearestFree();
    void onSelSaveF();
    void onWCF();
    void onProcessFocusMes();

};

#endif // CALIB_H
