#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
namespace cv{class Mat;}

class _predictor: public QObject{
    Q_OBJECT
public:
    cv::Mat eval(const pgScanGUI::scanRes* pre, cv::Point2f center, float centerHChange, double* intensity=nullptr, double* duration=nullptr);    //optionally returns the intensity and duration
    void    evalm(pgScanGUI::scanRes* pre, cv::Point2f center, float centerHChange, double* intensity=nullptr, double* duration=nullptr);         //this one modifies the received scanRes instead of just returning the changed mat
    QWidget* gui_settings;
protected:
    virtual void    _eval(const pgScanGUI::scanRes* pre, cv::Mat* post, cv::Point2f center, float centerHChange, double* intensity=nullptr, double* duration=nullptr)=0;
    QVBoxLayout* slayout;
};


class gaussian_predictor: public _predictor{
public:
    gaussian_predictor();
private:
    val_selector* pulseDur;
    val_selector* pulseFWHM;
    val_selector* bestSizeCutoffPercent;
    float gaussian(float x, float y, float a, float wx, float wy, float an=0);
    void _getBestSize(int* xSize, int* ySize, double XYnmppx);       //returns the ideal Mat size (relating to spot size), if smaller the writing spot may be cut off
    void _eval(const pgScanGUI::scanRes* pre, cv::Mat* post, cv::Point2f center, float centerHChange, double* intensity=nullptr, double* duration=nullptr);
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
