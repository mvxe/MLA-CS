#ifndef POSITION_REPORT_H
#define POSITION_REPORT_H

#include "PROCEDURES/procedure.h"
#include <QLabel>
#include <QTimer>
class QVBoxLayout;
class QHBoxLayout;
class pgMoveGUI;

class pgPosRepGUI: public QLabel{
    Q_OBJECT
    //GUI
public:
    pgPosRepGUI(pgMoveGUI* pgMGUI);
    QTimer *timer;
    constexpr static unsigned timer_delay=250;      //we refresh position every 250 ms
    double old[3]{0,0,0};
private:
    pgMoveGUI* pgMGUI;
private Q_SLOTS:
    void update();
    void showEvent(QShowEvent *){timer->start();}   //overriden
    void hideEvent(QHideEvent *){timer->stop();}    //overriden

Q_SIGNALS:
    void changed(double X, double Y, double Z);
};


#endif // POSITION_REPORT_H
