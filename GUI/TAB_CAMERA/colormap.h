#ifndef COLORMAP_H
#define COLORMAP_H
#include <QLabel>
#include "opencv2/opencv.hpp"
#include "UTIL/.rtoml/rtoml.hpp"
class smp_selector;
class val_selector;
class QVBoxLayout;
class pgScanGUI;
class pgTiltGUI;
class QPushButton;
class checkbox_gs;
class hidCon;

class colorMap: public QWidget{
    Q_OBJECT
public:
    rtoml::vsr conf;                                        //configuration map
    colorMap(smp_selector* cm_sel, cv::Scalar& exclColor, checkbox_gs* showAbsHeight, pgScanGUI* pgSGUI, pgTiltGUI* pgTGUI);
    void colormappize(const cv::Mat* src, cv::Mat* dst, const cv::Mat* mask, double min, double max, double XYnmppx, bool excludeOutOfRange=false, bool isForExport=false, std::string label="Height (nm)", bool isReflectivity=false);
    const bool& changed{_changed};
    void draw_bw_target(cv::Mat* src, float dX, float dY);
    void draw_bw_scalebar(cv::Mat* src, double XYnmppx, double xysbar_unit_override=0);
    void draw_color_box(cv::Mat* src, int x0, int x1, int y0, int y1);
    const std::atomic<bool>* exportSet4WholeVal;
private:
    QVBoxLayout* layout;
    smp_selector* fontFace;
    val_selector* fontSize;
    val_selector* fontSizeExport;
    val_selector* fontThickness;
    val_selector* barWidth;
    val_selector* barGap;
    val_selector* textOffset;
    val_selector* displayANTicks;
    val_selector* exportANTicks;
    val_selector* XYnmppx;  std::mutex glock;
    QPushButton* calibXY;
    val_selector* tilt;
    QPushButton* movTilt;

    val_selector* xysbar_unit;
    val_selector* xysbar_thck;
    val_selector* xysbar_txtoffset;
    smp_selector* xysbar_corner;
    val_selector* xysbar_xoffset;
    val_selector* xysbar_yoffset;
    checkbox_gs* xysbar_color_inv;

    hidCon* hc_export;
    checkbox_gs* exportSet4Whole;
    val_selector* xysbar_unit_Export;
    smp_selector* xysbar_corner_Export;
    val_selector* xysbar_xoffset_Export;
    val_selector* xysbar_yoffset_Export;


    double phiX0, phiY0, phiX1, phiY1, phiXR, phiYR;

    hidCon* hc_calib;
    smp_selector* cm_sel;
    checkbox_gs* showAbsHeight;
    cv::Scalar& exclColor;
    pgScanGUI* pgSGUI;      //for XY calibration
    pgTiltGUI* pgTGUI;      // ^^
    std::array<int,3> stdTicks{1,2,5}; //and further times 10
    bool _changed{false};
private Q_SLOTS:
    void onChanged();
    void onCalibrateXY(bool state);
    void onMovTilt(bool state);
};

#endif // COLORMAP_H
