#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
namespace cv{class Mat;}

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis();
    QWidget* gui_settings;
    QWidget* gui_activation;
    void getCalibWritingBeam(float* x, float* y, float* r, std::string radHistSaveFName="", float* dx=nullptr, float* dy=nullptr);

private:
    QVBoxLayout* slayout;
    val_selector* selMaxRoundnessDev;
    val_selector* selCannyThreshL;
    val_selector* selCannyThreshU;
    val_selector* selMinPixNum;
    QPushButton* btnSaveNextDebug;

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
};

#endif // BEAMANL_H
