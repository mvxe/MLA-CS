#ifndef MOVE_H
#define MOVE_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
class QVBoxLayout;
class QHBoxLayout;
class val_selector;
class eadScrlBar;

class pgMoveGUI: public QWidget{
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
};

#endif // MOVE_H
