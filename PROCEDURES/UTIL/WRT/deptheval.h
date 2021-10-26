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
    rtoml::vsr conf;                //configuration map
    const bool& debugChanged{_debugChanged};
    const pgScanGUI::scanRes* getDebugImage(const pgScanGUI::scanRes* src);
    cv::Mat getMaskFlatness(const pgScanGUI::scanRes* src, int dil, double thresh, double blur, int dily=-1);
                                                                                                            // returns a CV_U8 matrix containing the mask, 0 values represent flat areas, 255 curved
                                                                                                            // if dily, rect with (dil,dily) is used instead of ellipse(dil)
private:
    QVBoxLayout* layout;
    smp_selector* debugDisp;

    int debugIndex{0};
    bool _debugChanged{false};

    val_selector* findf_Blur;
    val_selector* findf_Thrs;
    val_selector* findf_Dill;
    val_selector* findf_Rand;
    val_selector* findf_Dilly;
    QPushButton* btnGoToNearestFree;

    pgScanGUI::scanRes res;
    pgBoundsGUI* pgBGUI;
private Q_SLOTS:
    void onDebugIndexChanged(int index);
    void onChanged();
    void onGoToNearestFree();
Q_SIGNALS:
    void sigGoToNearestFree(double radDilat, double radRandSpread, double blur, double thrs, double radDilaty, bool convpx2um);
};

#endif // DEPTHEVAL_H
