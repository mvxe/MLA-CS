#ifndef TILT_H
#define TILT_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
#include <QLabel>
class QPushButton;
class joyBttn;
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;
namespace cv{class Mat;}

class pgTiltGUI: public QWidget{
    Q_OBJECT
    friend joyBttn;
    //GUI
public:
    pgTiltGUI();
    QWidget* gui_activation;
    QWidget* gui_settings;

 private:
    void init_gui_activation();
    void init_gui_settings();

    //activation
    QVBoxLayout* alayout;
    cv::Mat* box;
    eadScrlBar* xTilt;
    eadScrlBar* yTilt;

    //settings
    QVBoxLayout* slayout;
    val_selector* tilt_mult;        //pixels time this gives the movement per unit interval
    val_selector* focus_autoadjX;   //how much is the Z adjusted per unit interval change of X
    val_selector* focus_autoadjY;
    QPushButton* calib_focus_autoadjX;  //autocalibrates the focus adjustment
    QPushButton* calib_focus_autoadjY;
    val_selector* tilt_motor_speed; //the speed of the tilt motor

    bool inited=false;
    double Tilt_cum_X, Tilt_cum_Y, Z_cum;
private Q_SLOTS:
    void work_fun(double magnitude, bool isX);
    void _work_fun_T(double magnitude){work_fun(magnitude, true);}
    void _work_fun_F(double magnitude){work_fun(magnitude, false);}
    void onCalibrate(bool isStart, bool isX);
    void _onCalibrate_X(bool isStart){onCalibrate(isStart, true);}
    void _onCalibrate_Y(bool isStart){onCalibrate(isStart, false);}
};


#endif // TILT_H
