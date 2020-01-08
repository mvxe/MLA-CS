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
    void updateImg(const pgScanGUI::scanRes* scanres, double* rmin=nullptr, double* rmax=nullptr, double altMin=0, double altMax=0, cv::Mat* altDepth=nullptr);       //the alt vars to override depth and minmax from res
    const double& hPcnt{_hPcnt};
    const double& lPcnt{_lPcnt};
    const bool& changed{_changed};
    const bool& ExclOOR{cbOORtE};
private:
    QLabel* imgDisp;
    QVBoxLayout* layout;
    QWidget* btnLW;
    QHBoxLayout* btnLayout;
    QDoubleSpinBox* _hPcnt_sel;
    QDoubleSpinBox* _lPcnt_sel;
    QCheckBox* outOfRangeToExcl;

    double _hPcnt, _lPcnt;
    bool cbOORtE;
    cc_save<double> sv_hPcnt{_hPcnt, 100,&go.gui_config.save,"pgHistogrameGUI_hPcnt"};
    cc_save<double> sv_lPcnt{_lPcnt,   0,&go.gui_config.save,"pgHistogrameGUI_lPcnt"};
    cc_save<bool> sv_outOfRangeToExcl{cbOORtE,false,&go.gui_config.save,"pgHistogrameGUI_outOfRangeToExcl"};

    bool _changed{false};
    smp_selector* cm_sel;
    int Hsize;
    int Vsize;
    cv::Scalar& exclColor;
private Q_SLOTS:
    void onValueChanged_hPcnt(double value);
    void onValueChanged_lPcnt(double value);
    void onValueChanged_cbox(int state);
};
#endif // HISTOGRAM_H
