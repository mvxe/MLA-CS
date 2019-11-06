#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class QLabel;
class smp_selector;
class QVBoxLayout;
class QHBoxLayout;
class QDoubleSpinBox;

class pgHistogrameGUI: public QWidget{
    Q_OBJECT
    //GUI
public:
    pgHistogrameGUI(int Hsize, int Vsize, cvMat_safe* cvms_img, cvMat_safe* cvms_mask, smp_selector* cm_sel, cv::Scalar& exclColor);        //cvms_mask is inverse mask!
    void updateImg(double* rmin=nullptr, double* rmax=nullptr);
    const double& hPcnt{_hPcnt};
    const double& lPcnt{_lPcnt};
    const bool& changed{_changed};
private:
    QLabel* imgDisp;
    QVBoxLayout* layout;
    QWidget* btnLW;
    QHBoxLayout* btnLayout;
    QDoubleSpinBox* _hPcnt_sel;
    QDoubleSpinBox* _lPcnt_sel;

    double _hPcnt, _lPcnt;
    cc_save<double> sv_hPcnt{_hPcnt, 100,&go.gui_config.save,"pgHistogrameGUI_hPcnt"};
    cc_save<double> sv_lPcnt{_lPcnt,   0,&go.gui_config.save,"pgHistogrameGUI_lPcnt"};
    bool _changed{false};
    cvMat_safe* cvms_img;
    cvMat_safe* cvms_mask;
    smp_selector* cm_sel;
    int Hsize;
    int Vsize;
    cv::Scalar& exclColor;
private Q_SLOTS:
    void onValueChanged_hPcnt(double value);
    void onValueChanged_lPcnt(double value);
};
#endif // HISTOGRAM_H
