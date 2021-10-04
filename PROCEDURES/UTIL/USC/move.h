#ifndef MOVE_H
#define MOVE_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
#include "DEV/controller.h"
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;
class QPushButton;
class moveDial;
class QLabel;
class checkbox_gs;
class smp_selector;
//class PVTobj;

class pgMoveGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgMoveGUI(smp_selector* selObjective);
    rtoml::vsr conf;            //configuration map
    QWidget* gui_activation;
    QWidget* gui_settings;

    std::atomic<bool> reqstNextClickPixDiff{false};
    void delvrNextClickPixDiff(double Dx, double Dy);

    double getNmPPx(int index=-1);          // -1 active microscope, 0 - force Mirau, 1 - force Writing
    double getAngCamToXMot(int index=-1);
    double getYMotToXMot();

public Q_SLOTS:
    void chooseObj(bool useMirau);
    void move(double Xmov, double Ymov, double Zmov);  // in mm, corrects Z on X and Y move

    void scaledMoveX(double magnitude);
    void scaledMoveY(double magnitude);
    void scaledMoveZ(double magnitude);

    void corCOMove(CTRL::CO& co, double Xmov, double Ymov, double Zmov, bool forceSkewCorrection=false);

private:
    void init_gui_activation();
    void init_gui_settings();

    //activation
    QVBoxLayout* alayout;
    cv::Mat* box;
    eadScrlBar* xMove;
    eadScrlBar* yMove;
    eadScrlBar* zMove;
    val_selector* mpow;    //to speed up movement by 10 to the power
    std::vector<moveDial*> moveDials;
    QPushButton* addDial;
    QPushButton* rmDial;

    //settings
    QVBoxLayout* slayout;
    val_selector* xMoveScale;
    val_selector* yMoveScale;
    val_selector* zMoveScale;
    val_selector* autoadjXZ;        //how much is the Z adjusted per X movement
    val_selector* autoadjYZ;
    QPushButton* calib_autoadjXZ;   //autocalibrates the focus adjustment
    QPushButton* calib_autoadjYZ;
    double X_cum, Y_cum, Z_cum;
    QPushButton* markObjDis;


    //in camera_tab
    smp_selector* selObjective;
    std::atomic<int> currentObjective{-1};
    double objectiveDisplacement[3]{0,0,0}; // this is Writing-Mirau {dX,dY,dZ}
    double tmpOD[3];

    QPushButton* markPointForCalib;
    QLabel* ptFeedback;
    QPushButton* calculateCalib;
    smp_selector* settingsObjective;

    QWidget* calibMirauW;
    QVBoxLayout* calibMirauL;
    val_selector* calibNmPPx;
    val_selector* calibAngCamToXMot;

    QWidget* calibWritingW;
    QVBoxLayout* calibWritingL;
    val_selector* calibNmPPx_writing;
    val_selector* calibAngCamToXMot_writing;

    val_selector* calibAngYMotToXMot;
    struct dpoint{
        double DXpx, DYpx;
        double DXmm, DYmm;
    };
    dpoint curP4calib;
    std::vector<dpoint> p4calib;
    checkbox_gs* disableSkewCorrection;

    bool lastindex;
private Q_SLOTS:
    void onCalibrate(bool isStart, bool isX);
    void _onCalibrate_X(bool isStart){onCalibrate(isStart, true);}
    void _onCalibrate_Y(bool isStart){onCalibrate(isStart, false);}
    void onAddDial();
    void onRmDial();
    void onDialMove(double x,double y);
    void onMarkPointForCalib(bool state);
    void onCalculateCalib();
    void _chooseObj(int index);
    void _onMarkObjDisY(bool isStart);
    void onSettingsObjectiveChange(int index);

Q_SIGNALS:
    void sigChooseObj(bool useMirau);
    void sigChooseObjExpo(bool useMirau);
};

#endif // MOVE_H
