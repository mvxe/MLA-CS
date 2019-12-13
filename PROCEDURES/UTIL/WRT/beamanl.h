#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
namespace cv{class Mat;}

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis();
    QWidget* gui_activation;

private:
    QVBoxLayout* alayout;
    QPushButton* btnGetCenter;

    struct spot{
        float x,y,r;
        float dx,dy,dd;
    };
    std::mutex spotLock;    //for multithreading solve
    void solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone);
    static bool sortSpot(spot i,spot j);
    const float maxRoundnessDev{0.1f};

private Q_SLOTS:
    void getWritingBeamCenter();
};

#endif // BEAMANL_H
