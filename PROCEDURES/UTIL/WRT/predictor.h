#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
namespace cv{class Mat;}

class _predictor: public QObject{
    Q_OBJECT
public:
    virtual void getBestSize(int* xSize, int* ySize, double XYnmppx)=0;       //returns the ideal Mat size (relating to spot size), if smaller the writing spot may be cut off
    virtual cv::Mat eval(const pgScanGUI::scanRes* pre, double centerHChange, double* intensity=nullptr, double* duration=nullptr)=0;    //optionally returns the intensity and duration
    virtual void    evalm(pgScanGUI::scanRes* pre, double centerHChange, double* intensity=nullptr, double* duration=nullptr)=0;         //this one modifies the received scanRes instead of just returning the changed mat
    QWidget* gui_settings;
protected:
    QVBoxLayout* slayout;
};


class gaussian_predictor: public _predictor{
public:
    gaussian_predictor();
    void getBestSize(int* xSize, int* ySize, double XYnmppx);
    cv::Mat eval(const pgScanGUI::scanRes* pre, double centerHChange, double* intensity=nullptr, double* duration=nullptr);
    void evalm(pgScanGUI::scanRes* pre, double centerHChange, double* intensity=nullptr, double* duration=nullptr);
private:
    val_selector* pulseDur;
    val_selector* pulseFWHM;
    val_selector* bestSizeCutoffPercent;
    double gaussian(double x, double y, double a, double wx, double wy, double an=0);
};


class pgWritePredictor: public QObject{
    Q_OBJECT
public:
    pgWritePredictor(pgMoveGUI* pgMGUI);
    const bool& debugChanged{_debugChanged};
    const pgScanGUI::scanRes* getDebugImage(const pgScanGUI::scanRes* src); //for debugging

    QWidget* gui_settings;
    twds_selector* predictorSel;
    QPushButton* test;
    val_selector* testTargetHeight;
    gaussian_predictor prd_gaussian;
    bool _debugChanged{false};
private:
    QVBoxLayout* slayout;
    QLabel* status;
    pgMoveGUI* pgMGUI;
    pgScanGUI::scanRes res; //for debug
private Q_SLOTS:
    void onTest();
};



#endif // PREDICTOR_H
