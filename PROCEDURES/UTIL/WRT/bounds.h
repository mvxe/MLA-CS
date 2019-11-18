#ifndef BOUNDS_H
#define BOUNDS_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include <QTimer>

class pgBoundsGUI: public QWidget{
    Q_OBJECT
public:
    pgBoundsGUI();
    bool isWithinBounds(double x, double y);
private:
    QTimer* timer;
    constexpr static unsigned timer_delay=250;      //we refresh out of bounds indicator every this ms

    QVBoxLayout* layout;
    twd_selector* selector;
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
    QVBoxLayout* layRectPts;


    cc_save<double> saveCircCenterX{circCenter[0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX"};
    cc_save<double> saveCircCenterY{circCenter[1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY"};
    cc_save<double> saveCirccircEdgeX0{circEdge[0][0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX0"};
    cc_save<double> saveCirccircEdgeX1{circEdge[1][0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX1"};
    cc_save<double> saveCirccircEdgeX2{circEdge[2][0], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterX2"};
    cc_save<double> saveCirccircEdgeY0{circEdge[0][1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY0"};
    cc_save<double> saveCirccircEdgeY1{circEdge[1][1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY1"};
    cc_save<double> saveCirccircEdgeY2{circEdge[2][1], 0,&go.gui_config.save,"pgBoundsGUI_saveCircCenterY2"};

    std::string getStatCirc();
    void calcCenRad(double &x, double &y, double &rad);
private Q_SLOTS:
    void onSetCircCenter();
    void onSetCircEdge();
    void showEvent(QShowEvent *){timer->start();}   //overriden
    void hideEvent(QHideEvent *){timer->stop();}    //overriden
    void update();
};

#endif // BOUNDS_H
