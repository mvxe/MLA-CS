#ifndef TILT_H
#define TILT_H

#include "PROCEDURES/procedure.h"
#include <QLabel>
class QPushButton;
class joyBttn;
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;
class pgMoveGUI;
class smp_selector;
namespace cv{class Mat;}

class pgTiltGUI: public QObject{
    Q_OBJECT
    friend joyBttn;
    //GUI
public:
    pgTiltGUI(pgMoveGUI* pgMGUI);
    rtoml::vsr conf;                //configuration map
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
    //presets
    smp_selector* tiltPreset;
    QPushButton* tiltPresetAdd;
    QPushButton* tiltPresetRemove;
    std::vector<std::string> vec_pnames;
    std::vector<double> vec_pxtilt;
    std::vector<double> vec_pytilt;


    //settings
    QVBoxLayout* slayout;
    val_selector* tilt_mult;        //pixels time this gives the movement per unit interval
    val_selector* focus_autoadjX;   //how much is the Z adjusted per unit interval change of X
    val_selector* focus_autoadjY;

    QPushButton* calib_focus_autoadjX;  //autocalibrates the focus adjustment
    QPushButton* calib_focus_autoadjY;
    bool inited=false;
    double Tilt_cum_X, Tilt_cum_Y, Z_cum;

    pgMoveGUI* pgMGUI;

public Q_SLOTS:
    void doTilt(double magnitudeX, double magnitudeY, bool scale=true);
private Q_SLOTS:
    void _doTilt_X(double magnitude){doTilt(magnitude,0);}
    void _doTilt_Y(double magnitude){doTilt(0,magnitude);}
    void onCalibrate(bool isStart, bool isX);
    void _onCalibrate_X(bool isStart){onCalibrate(isStart, true);}
    void _onCalibrate_Y(bool isStart){onCalibrate(isStart, false);}
    void onTiltPresetAdd();
    void onTiltPresetRemove();
    void onTiltPreset();
    void onUpdateMenu();
};


#endif // TILT_H
