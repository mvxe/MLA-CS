#ifndef COLORMAP_H
#define COLORMAP_H
#include <QLabel>
#include "opencv2/opencv.hpp"
class smp_selector;
class val_selector;
class QVBoxLayout;

class colorMap: public QWidget{
    Q_OBJECT
public:
    colorMap(smp_selector* cm_sel, cv::Scalar& exclColor);
    void colormappize(cv::Mat* src, cv::Mat* dst, cv::Mat* mask, double min, double max, bool excludeOutOfRange=false, bool isForExport=false);
    const bool& changed{_changed};
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

    smp_selector* cm_sel;
    cv::Scalar& exclColor;
    std::array<int,3> stdTicks{1,2,5}; //and further times 10
    bool _changed{false};
private Q_SLOTS:
    void onChanged();
};

#endif // COLORMAP_H
