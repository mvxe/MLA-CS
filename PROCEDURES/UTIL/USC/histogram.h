#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
#include "scan.h"
class QLabel;
class smp_selector;
class QVBoxLayout;
class QHBoxLayout;
class QDoubleSpinBox;
class QCheckBox;

class pgHistogrameGUI: public QWidget{
    Q_OBJECT
    //GUI
public:
    pgHistogrameGUI(int Hsize, int Vsize, smp_selector* cm_sel, cv::Scalar& exclColor);        //cvms_mask is inverse mask!
    rtoml::vsr conf;                                        //configuration map
    void updateImg(const pgScanGUI::scanRes* scanres, double* rmin=nullptr, double* rmax=nullptr, double altMin=0, double altMax=0, cv::Mat* altDepth=nullptr);       //the alt vars to override depth and minmax from res

    const bool& changed{_changed};
    val_selector* hPcnt;
    val_selector* lPcnt;
    checkbox_gs* outOfRangeToExcl;
private:
    QLabel* imgDisp;
    QVBoxLayout* layout;
    QWidget* btnLW;
    QHBoxLayout* btnLayout;

    bool _changed{false};
    smp_selector* cm_sel;
    int Hsize;
    int Vsize;
    cv::Scalar& exclColor;
private Q_SLOTS:
    void onValueChanged_hPcnt(double value);
    void onValueChanged_lPcnt(double value);
    void onValueChanged_cbox(bool state);
};
#endif // HISTOGRAM_H
