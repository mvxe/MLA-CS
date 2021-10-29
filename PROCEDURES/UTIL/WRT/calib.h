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

    //settings
    QVBoxLayout* slayout;
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
    smp_selector* selPrerunType;
    val_selector* selPlateauA;
    val_selector* selPlateauB;
    val_selector* selPeakXshift;
    val_selector* selPeakYshift;
    smp_selector* selMultiArrayType;
    val_selector* multiarrayN;
    val_selector* selArrayFocusBlur;
    val_selector* selArrayFocusThresh;
    std::string lastFolder{""};
    HQPushButton* btnWriteCalib;

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
    constexpr static int maxRedoRefocusTries=3;

    void WCFArray(std::string folder);
    void WCFArrayOne(cv::Mat WArray, double plateau, cv::Rect ROI, cv::Rect sROI, std::string folder, double progressfac, bool isPlateau, double peakXshift, double peakYshift);
    void saveConf(std::string filename, double duration, double focus, double plateau, double peak, double peakXshift, double peakYshift);
    void saveMainConf(std::string filename);
    void selArray(int ArrayIndex, int MultiArrayIndex);
private Q_SLOTS:
    void onWCF();
    void onProcessFocusMes();
    void onChangeDrawWriteAreaOn(bool status);
    void onSelArrayTypeChanged(int index);
    void onSelMultiArrayTypeChanged(int index);
    void onMultiarrayNChanged(double val);
    void onPrerunTypeChanged(int);

};

#endif // CALIB_H
