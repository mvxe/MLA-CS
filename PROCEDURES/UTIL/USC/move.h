#ifndef MOVE_H
#define MOVE_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;
class QPushButton;

class pgMoveGUI: public QObject{
    Q_OBJECT
    //GUI
public:
    pgMoveGUI();
    QWidget* gui_activation;
    QWidget* gui_settings;

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

    double FZdifCur=-9999;
    bool ignoreNext=false;

private Q_SLOTS:
    void _onMoveX(double magnitude){onMove(magnitude,0,0,0);}
    void _onMoveY(double magnitude){onMove(0,magnitude,0,0);}
    void _onMoveZ(double magnitude){onMove(0,0,magnitude,0);}
    void _onMoveF(double magnitude){onMove(0,0,0,magnitude);}
    void _onMoveZF(double difference);
    void onLockF(bool locked);
    void onMove(double Xmov, double Ymov, double Zmov, double Fmov);
    void onFZdifChange(double X, double Y, double Z, double F);
    void onCalibrate(bool isStart, bool isX);
    void _onCalibrate_X(bool isStart){onCalibrate(isStart, true);}
    void _onCalibrate_Y(bool isStart){onCalibrate(isStart, false);}
};

#endif // MOVE_H
