#ifndef BOUNDS_H
#define BOUNDS_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include <QTimer>
namespace cv{class Mat;}

class pgBoundsGUI: public QWidget{
    Q_OBJECT
public:
    pgBoundsGUI();
    ~pgBoundsGUI();
    bool isWithinBounds(double x, double y);
    void drawBound(cv::Mat* img, double XYnmppx, bool isMask=false);    //if isMask, the values outside the zone are turned to 255
private:
    QTimer* timer;
    constexpr static unsigned timer_delay=250;      //we refresh out of bounds indicator every this ms

    QVBoxLayout* layout;
    twd_selector* selector;
    int index;
    QLabel* OOBLabel;
    QWidget* circRad;
    QWidget* circPts;
    QWidget* rectDim;
    QWidget* rectPts;
    QVBoxLayout* layCircRad;
        QPushButton* setCircCenter;
        QLabel* setCircCenterTxt;
        double circCenter[2];
        val_selector* selCircRadius;
    QVBoxLayout* layCircPts;
        QPushButton* setCircEdge;
        QLabel* setCircEdgeTxt;
        double circEdge[3][2];
        int cIter{0};
        val_selector* circClearance;
    QVBoxLayout* layRectDim;
        QPushButton* setRectCenter;
        QLabel* setRectCenterTxt;
        double rectCenter[2];
        val_selector* selRectWidth;
        val_selector* selRectHeight;
    QVBoxLayout* layRectPts;
        QPushButton* setRectEdge;
        QLabel* setRectEdgeTxt;
        double rectEdge[4][2];
        int rIter{0};
        val_selector* rectClearance;

    cc_save<double> saveCircCenterX{circCenter[0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX"};
    cc_save<double> saveCircCenterY{circCenter[1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY"};
    cc_save<double> saveCircEdgeX0{circEdge[0][0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX0"};
    cc_save<double> saveCircEdgeX1{circEdge[1][0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX1"};
    cc_save<double> saveCircEdgeX2{circEdge[2][0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX2"};
    cc_save<double> saveCircEdgeY0{circEdge[0][1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY0"};
    cc_save<double> saveCircEdgeY1{circEdge[1][1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY1"};
    cc_save<double> saveCircEdgeY2{circEdge[2][1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY2"};
    cc_save<double> saveRectCenterX{rectCenter[0], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterX"};
    cc_save<double> saveRectCenterY{rectCenter[1], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterY"};
    cc_save<double> saveRectEdgeX0{rectEdge[0][0], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterX0"};
    cc_save<double> saveRectEdgeX1{rectEdge[1][0], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterX1"};
    cc_save<double> saveRectEdgeX2{rectEdge[2][0], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterX2"};
    cc_save<double> saveRectEdgeX3{rectEdge[3][0], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterX3"};
    cc_save<double> saveRectEdgeY0{rectEdge[0][1], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterY0"};
    cc_save<double> saveRectEdgeY1{rectEdge[1][1], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterY1"};
    cc_save<double> saveRectEdgeY2{rectEdge[2][1], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterY2"};
    cc_save<double> saveRectEdgeY3{rectEdge[3][1], 0,&go.gui_config.save,"pgBoundsGUI_saveRectCenterY3"};
    cc_save<int> saveIndex{index, 0,&go.gui_config.save,"pgBoundsGUI_index"};
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
