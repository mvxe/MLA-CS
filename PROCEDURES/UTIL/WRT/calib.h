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
class CvPlotQWindow;
class QSlider;
namespace CvPlot{class Window;}
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

class pgCalib: public QWidget{
    Q_OBJECT
public:
    pgCalib(pgScanGUI* pgSGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgBeamAnalysis* pgBeAn, pgWrite* pgWr, overlay& ovl);
    ~pgCalib();
    rtoml::vsr conf;                //configuration map

    QWidget* gui_settings;
    QWidget* gui_settings_proc;
    void drawWriteArea(cv::Mat* img);
private:
    pgFocusGUI* pgFGUI;
    pgMoveGUI* pgMGUI;
    pgScanGUI* pgSGUI;
    pgBeamAnalysis* pgBeAn;
    pgWrite* pgWr;
    overlay& ovl;

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
    checkbox_gs* saveRF;
    smp_selector* selPrerunType;
    val_selector* selPlateauA;
    val_selector* selPlateauB;
    val_selector* selPeakXshift;
    val_selector* selPeakYshift;
    smp_selector* selMultiArrayType;
    val_selector* multiarrayN;
    std::string lastFolder{""};
    struct scheduled{
        void* ovlptr;
        double pos[3];
    };
    std::list<scheduled> scheduledPos;

    HQPushButton* btnWriteCalib;
    HQPushButton* scheduleMultiWrite;
    QLabel* report;

    int measCounter{0};
    bool drawWriteAreaOn{false};

    //processing
    QVBoxLayout* splayout;
    QPushButton* btnProcessFocusMes;

    QPushButton* fpLoad;
    QPushButton* fpClear;
    QPushButton* fitAndPlot;
    QPushButton* fpApply;
    QMenu* applyMenu;
    val_selector* nBSplineCoef;
    checkbox_gs* showBP;
    checkbox_gs* optimizeBP;
    QSlider* upperLim;
    QLabel* fpList;
    CvPlotQWindow* cpwin{nullptr};
    struct durhe_data{
        double duration;
        double height;
        double height_err;
    };
    std::vector<durhe_data> fpLoadedData;
    bool fpLoadedDataSorted;
    double focus[3];    // {focus, xofs, yofs}
    std::vector<double> bsplbreakpts;   // last calculated coefs
    std::vector<double> bsplcoefs;
    std::vector<double> bsplcov;
    double linA,linX;
    constexpr static int maxRedoScanTries=3;
    constexpr static int maxRedoRefocusTries=3;

    void WCFArray(std::string folder);
    bool WCFArrayOne(cv::Mat WArray, double plateau, cv::Rect ROI, cv::Rect sROI, std::string folder, bool isPlateau, double peakXshift, double peakYshift, unsigned n);
    void saveConf(std::string filename, double duration, double focus, double plateau, double peak, double peakXshift, double peakYshift);
    void saveMainConf(std::string filename);
    void selArray(int ArrayIndex, int MultiArrayIndex);
    void calcParameters(std::string fldr, std::string* output, std::atomic<unsigned>* completed);
private Q_SLOTS:
    void onWCF();
    void onSchedule();
    void onProcessFocusMes();
    void onChangeDrawWriteAreaOn(bool status);
    void onChangeDrawWriteAreaOnSch(bool status);
    void onSelArrayTypeChanged(int index);
    void onSelMultiArrayTypeChanged(int index);
    void onMultiarrayNChanged(double val);
    void onPrerunTypeChanged(int);
    void onFitAndPlot();
    void onfpLoad();
    void onfpClear();
    void onNBSplineCoefChanged();
    void onApply();
    void onApplyMenuTriggered(QAction* action);

private:
    static int gauss2De_f (const gsl_vector* pars, void* data, gsl_vector* f);
    static int gauss2De_df (const gsl_vector* pars, void* data, gsl_matrix* J);
    struct fit_data {
        size_t n;
        double* x;
        double* y;
        double* h;
    };
    static int bspline_f(const gsl_vector* pars, void* data, gsl_vector* f=nullptr);
    struct fit_data_och {
        size_t n;
        size_t ncoeffs;
        double* x;
        double* y;
        double* w;
        double* breakpts;
        double* coefs;
        double* covmat;
    };
};

#endif // CALIB_H
