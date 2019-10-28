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
namespace cv{class Mat;}

class pgTiltGUI: public QWidget{
    Q_OBJECT
    friend joyBttn;
    //GUI
public:
    pgTiltGUI();
    QWidget* gui_activation;
    QWidget* gui_settings;
    QTimer* timer;
    void reDraw0();

 private:
    void init_gui_activation();
    void init_gui_settings();
    void reDraw(int x, int y);

    //activation
    QHBoxLayout* alayout;
    joyBttn* joyBtn;
    constexpr static unsigned boxSize=50;
    constexpr static unsigned limSize=7;
    cv::Mat* box;

    //settings
    QVBoxLayout* slayout;
    val_selector* tilt_mult;        //pixels time this gives the movement per unit interval
    val_selector* focus_autoadjX;   //how much is the Z adjusted per unit interval of X
    val_selector* focus_autoadjY;   //how much is the Z adjusted per unit interval of Y

    int Xmov{0},Ymov{0};
    constexpr static unsigned work_call_time=500;

    bool inited=false;
private Q_SLOTS:
    void work_fun();
};


class joyBttn : public QLabel{
    using QLabel::QLabel;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
public:
    pgTiltGUI* parent;
};

#endif // TILT_H
