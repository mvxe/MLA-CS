#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
namespace cv{class Mat;}
class pgMoveGUI;

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis(pgMoveGUI* pgMGUI);
    ~pgBeamAnalysis();
    QWidget* gui_settings;
    QWidget* gui_activation;
    void getCalibWritingBeam(float* r, float* dx=nullptr, float* dy=nullptr);
    const float& writeBeamCenterOfsX{_writeBeamCenterOfsX};
    const float& writeBeamCenterOfsY{_writeBeamCenterOfsY};
private:
    pgMoveGUI* pgMGUI;
    float _writeBeamCenterOfsX;        //the center offset in pixels
    float _writeBeamCenterOfsY;
    cc_save<float> saveWBCX{_writeBeamCenterOfsX, 0,&go.pos_config.save,"pgBeamAnalysis_saveWBCX"};
    cc_save<float> saveWBCY{_writeBeamCenterOfsY, 0,&go.pos_config.save,"pgBeamAnalysis_saveWBCY"};

    QVBoxLayout* slayout;
    QPushButton* btnReset;
    twd_selector* method_selector;
    int methodIndex;
    cc_save<int> saveIndex{methodIndex, 0,&go.gui_config.save,"pgBeamAnalysis_methodindex"};
        QWidget* methodSimple;
        QVBoxLayout* methodSimpleLayout;
            val_selector* selThresh;
            val_selector* avgNum;
        QWidget* methodEllipsse;
        QVBoxLayout* methodEllipsseLayout;
            val_selector* selMaxRoundnessDev;
            val_selector* selCannyThreshL;
            val_selector* selCannyThreshU;
            val_selector* selMinPixNum;
    QPushButton* btnSaveNextDebug;
    val_selector* extraOffsX;
    val_selector* extraOffsY;
//    val_selector* cameraExposure;

    QVBoxLayout* alayout;
    QPushButton* btnGetCenter;

    struct spot{
        float x,y,r;
        float dx,dy,dd;
    };
    std::mutex spotLock;    //for multithreading solve
    void solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone);
    static bool sortSpot(spot i,spot j);
    std::string saveNext{""};
private Q_SLOTS:
    void getWritingBeamCenter();
    void onBtnSaveNextDebug();
    void onBtnReset();
};

#endif // BEAMANL_H
