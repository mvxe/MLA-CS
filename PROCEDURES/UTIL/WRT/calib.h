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
class pgWrite;
class QSpinBox;

class pgCalib: public QWidget{
    Q_OBJECT
public:
    pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv, pgBeamAnalysis* pgBeAn, pgWrite* pgWr);
    rtoml::vsr conf;                //configuration map

    QWidget* gui_activation;
    QWidget* gui_settings;
    QWidget* gui_processing;
    void drawWriteArea(cv::Mat* img);
private Q_SLOTS:
    bool goToNearestFree(double radDilat, double radRandSpread, double blur, double thrs, double radDilaty=0, bool convpx2um=false);      //returns true if failed (no free point nearby)
private:
    pgFocusGUI* pgFGUI;
    pgMoveGUI* pgMGUI;
    pgBoundsGUI* pgBGUI;
    pgScanGUI* pgSGUI;
    pgDepthEval* pgDpEv;
    pgBeamAnalysis* pgBeAn;
    pgWrite* pgWr;

    //activation
    QVBoxLayout* alayout;
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
        checkbox_gs* prewritePlateau;
        val_selector* selPlateauA;
        val_selector* selPlateauB;

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
    std::string makeDateTimeFolder(const std::string folder);
    void saveConf(std::string filename, double duration, double focus, double plateau);
    void saveMainConf(std::string filename);
private Q_SLOTS:
    void onWCF();
    void onProcessFocusMes();
    void onChangeDrawWriteAreaOn(bool status);
    void onSelArrayTypeChanged(int index);

};

#endif // CALIB_H
