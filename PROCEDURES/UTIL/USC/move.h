#ifndef MOVE_H
#define MOVE_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
#include "DEV/controller.h"
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
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

    const std::atomic<int>& currentObj{currentObjective};   // 0-mirau, 1-writing, -1-not initialized
    const double &objectiveDisplacementX{_objectiveDisplacement[0]};
    const double &objectiveDisplacementY{_objectiveDisplacement[1]};
    const double &objectiveDisplacementZ{_objectiveDisplacement[2]};

    // How it works; there are three(four) types of XY coordinates:
    //      - stage X, Y (these are skewed)
    //      - corrected X, Y (skew corrected and angle to (Mirau objective) camera corrected)
    //                  move() and corCOMove() applies this correction to get raw movements
    //                  you can also use Xcor(Xraw,Yraw) and Ycor(Xraw,Yraw)
    //                  or for reverse Xraw(Xcor,Ycor) and Yraw(Xcor,Ycor)
    //      - objective X, Y, in pixels
    //                  to convert use mm2px(coord) and px2mm(coord), where coord is corrected X or Y
    // for converting X,Y coordinates to/from mm from/to px
    double Xraw(double Xcor, double Ycor, bool useWritingObjAngles=false);      // useWritingObjAngles is used only in click to move in writing obj mode
    double Yraw(double Xcor, double Ycor, bool useWritingObjAngles=false);      // we neglect this in area displays in writing obj mode though as they are nearly the same
    double Xcor(double Xraw, double Yraw, bool useWritingObjAngles=false);
    double Ycor(double Xraw, double Yraw, bool useWritingObjAngles=false);
    double mm2px(double coord, int objective_index_override=-1, double nmppx=0);    // here you can also manually override nmppx
    double px2mm(double coord, int objective_index_override=-1, double nmppx=0);

    void getPos(double* X, double* Y=nullptr, double* Z=nullptr);       // returns current position, corrected by obj-obj difference
public Q_SLOTS:
    void chooseObj(bool useMirau);
    void move(double Xmov, double Ymov, double Zmov, bool isAbsolute=false, bool useWritingObjAngles=false);    // in mm, corrects Z on X and Y move

    void scaledMoveX(double magnitude);
    void scaledMoveY(double magnitude);
    void scaledMoveZ(double magnitude);

    void corCOMove(CTRL::CO& co, double Xmov, double Ymov, double Zmov, bool forceSkewCorrection=false);
    void wait4motionToComplete();
private:
    void init_gui_activation();
    void init_gui_settings();

    double _objectiveDisplacement[3]{0,0,0};                 // this is Writing-Mirau {dX,dY,dZ}

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
    QPushButton* gotoX;
    QPushButton* gotoY;
    QPushButton* gotoZ;

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

    double a[2],b[2],c[2],d[2],A[2],B[2],C[2],D[2];

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
    void reCalcConst(bool isMirau);
    void reCalcConstM(){reCalcConst(true);};
    void reCalcConstW(){reCalcConst(false);};
    void onGotoX();
    void onGotoY();
    void onGotoZ();
Q_SIGNALS:
    void sigChooseObj(bool useMirau);
    void sigChooseObjExpo(bool useMirau);
private:
    static int calibFit_f(const gsl_vector* pars, void* data, gsl_vector* f);
    struct fit_data {
        size_t n;
        double* DXpx;
        double* DYpx;
        double* DXmm;
        double* DYmm;
    };
};

#endif // MOVE_H
