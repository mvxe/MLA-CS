#ifndef MOVE_H
#define MOVE_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;
class QPushButton;
class moveDial;
class QLabel;
class checkbox_save;
class PVTobj;

class pgMoveGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgMoveGUI();
    QWidget* gui_activation;
    QWidget* gui_settings;

    std::atomic<bool> reqstNextClickPixDiff{false};
    void delvrNextClickPixDiff(double Dx, double Dy);

    double getNmPPx();
    double getAngCamToXMot();
    double getYMotToXMot();
    std::atomic<double>& FZdifference{FZdifCur};

public Q_SLOTS:
    void moveZF(double difference);                                                                 //in mm
    void move(double Xmov, double Ymov, double Zmov, double Fmov, bool forceSkewCorrection=false);  //in mm, corrects ZF on X and Y move

    void scaledMoveX(double magnitude);
    void scaledMoveY(double magnitude);
    void scaledMoveZ(double magnitude);
    void scaledMoveF(double magnitude);

    void corPvt(PVTobj* po, double time, double Xmov, double Xspd, double Ymov, double Yspd, double Zmov=0, double Zspd=0, double Fmov=0, double Fspd=0, bool forceSkewCorrection=false);
                                // for adding corrected pvt segments. NOTE: this corrects ZF on X and Y move, and moving Z also moves F, i.e. the Fmov, Fspd are actually (F-Z) movements
private:

    void init_gui_activation();
    void init_gui_settings();

    //activation
    QVBoxLayout* alayout;
    cv::Mat* box;
    eadScrlBar* xMove;
    eadScrlBar* yMove;
    eadScrlBar* zMove;
    eadScrlBar* fMove;
    val_selector* FZdif;
    val_selector* mpow;    //to speed up movement by 10 to the power
    std::vector<moveDial*> moveDials;
    QPushButton* addDial;
    QPushButton* rmDial;

    //settings
    QVBoxLayout* slayout;
    val_selector* xMoveScale;
    val_selector* yMoveScale;
    val_selector* zMoveScale;
    val_selector* fMoveScale;
    val_selector* autoadjXZ;        //how much is the Z adjusted per X movement
    val_selector* autoadjYZ;
    QPushButton* calib_autoadjXZ;   //autocalibrates the focus adjustment
    QPushButton* calib_autoadjYZ;
    double X_cum, Y_cum, Z_cum;

    QPushButton* markPointForCalib;
    QLabel* ptFeedback;
    QPushButton* calculateCalib;
    val_selector* calibNmPPx;
    val_selector* calibAngCamToXMot;
    val_selector* calibAngYMotToXMot;
    struct dpoint{
        double DXpx, DYpx;
        double DXmm, DYmm;
    };
    dpoint curP4calib;
    std::vector<dpoint> p4calib;
    checkbox_save* skewCorrection;

    std::atomic<double> FZdifCur{-9999};

private Q_SLOTS:
    void onLockF(bool locked);
    void onFZdifChange(double X, double Y, double Z, double F);
    void onCalibrate(bool isStart, bool isX);
    void _onCalibrate_X(bool isStart){onCalibrate(isStart, true);}
    void _onCalibrate_Y(bool isStart){onCalibrate(isStart, false);}
    void onAddDial();
    void onRmDial();
    void onDialMove(double x,double y);
    void onMarkPointForCalib(bool state);
    void onCalculateCalib();
};

#endif // MOVE_H
