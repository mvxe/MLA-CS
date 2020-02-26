#ifndef DEPTHEVAL_H
#define DEPTHEVAL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
class pgBoundsGUI;

class pgDepthEval: public QWidget{
    Q_OBJECT
public:
    pgDepthEval(pgBoundsGUI* pgBGUI);
    const bool& debugChanged{_debugChanged};
    const pgScanGUI::scanRes* getDebugImage(const pgScanGUI::scanRes* src);
    cv::Mat getMaskFlatness(const pgScanGUI::scanRes* src, int dil=-1, double thresh=-1, double blur=-1);   //if dil, thresh and/or blur arent specified, the ones in pgDepthEval settings are used
                                                                                                            //returns a CV_U8 matrix containing the mask, 0 values represent flat areas, 255 curved
    cv::Mat getMaskBoundary(const pgScanGUI::scanRes* src, int dilx=0, int dily=0);                         //the dil square size overlayed over every point is cv::Size(2*dilx+1,2*dily+1)
                                                                                                            //returned mask does NOT include anything from the original mask
private:
    QVBoxLayout* layout;
    smp_selector* debugDisp;

    int debugIndex{0};
    bool _debugChanged{false};

    val_selector* findf_Blur;
    val_selector* findf_Thrs;
    val_selector* findf_Dill;

    pgScanGUI::scanRes res;
    pgBoundsGUI* pgBGUI;
private Q_SLOTS:
    void onDebugIndexChanged(int index);
    void onChanged();
};

#endif // DEPTHEVAL_H
