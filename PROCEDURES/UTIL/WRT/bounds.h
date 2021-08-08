#ifndef BOUNDS_H
#define BOUNDS_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include <QTimer>
namespace cv{class Mat;}
class pgMoveGUI;
class pgBeamAnalysis;

class pgBoundsGUI: public QWidget{
    Q_OBJECT
public:
    pgBoundsGUI(pgMoveGUI* pgMGUI, pgBeamAnalysis* pgBeAn);
    ~pgBoundsGUI();
    rtoml::vsr conf;                //configuration map
    bool isWithinBounds(double x, double y);
    void drawBound(cv::Mat* img, double XYnmppx, bool isMask=false);    //if isMask, the values outside the zone are turned to 255
private:
    pgMoveGUI* pgMGUI;
    pgBeamAnalysis* pgBeAn;
    QTimer* timer;
    constexpr static unsigned timer_delay=250;      //we refresh out of bounds indicator every this ms

    QVBoxLayout* layout;
    twd_selector* selector;
    int index{0};
    QLabel* OOBLabel;
    QWidget* circRad;
    QWidget* circPts;
    QWidget* rectDim;
    QWidget* rectPts;
    QVBoxLayout* layCircRad;
        QPushButton* setCircCenter;
        QLabel* setCircCenterTxt;
        double circCenter[2]{0,0};
        val_selector* selCircRadius;
    QVBoxLayout* layCircPts;
        QPushButton* setCircEdge;
        QLabel* setCircEdgeTxt;
        double circEdge[3][2]{{0,0},{0,0},{0,0}};
        int cIter{0};
        val_selector* circClearance;
    QVBoxLayout* layRectDim;
        QPushButton* setRectCenter;
        QLabel* setRectCenterTxt;
        double rectCenter[2]{0,0};
        val_selector* selRectWidth;
        val_selector* selRectHeight;
    QVBoxLayout* layRectPts;
        QPushButton* setRectEdge;
        QLabel* setRectEdgeTxt;
        double rectEdge[4][2]{{0,0},{0,0},{0,0},{0,0}};
        int rIter{0};
        val_selector* rectClearance;

    double cur[2]{0,0};

    std::string getStatCirc();
    std::string getStatRect();
    void calcCenRad(double &x, double &y, double &rad);
    bool isWithinRect(double x, double y);
    void getLineDisDir(double a, double b, double x, double y, bool* dir, double* dis=nullptr);
private Q_SLOTS:
    void onSetCircCenter();
    void onSetCircEdge();
    void onSetRectCenter();
    void onSetRectEdge();
    void showEvent(QShowEvent *){timer->start();}   //overriden
    void hideEvent(QHideEvent *){timer->stop();}    //overriden
    void update();
};

#endif // BOUNDS_H
