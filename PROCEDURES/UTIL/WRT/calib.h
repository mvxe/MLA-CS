#ifndef CALIB_H
#define CALIB_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
class pgFocusGUI;
class pgMoveGUI;
class pgBoundsGUI;
class pgDepthEval;

class pgCalib: public QWidget{
    Q_OBJECT
public:
    pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv);
    ~pgCalib();

    QWidget* gui_activation;
    QWidget* gui_settings;

private:
    pgFocusGUI* pgFGUI;
    pgMoveGUI* pgMGUI;
    pgBoundsGUI* pgBGUI;
    pgScanGUI* pgSGUI;
    pgDepthEval* pgDpEv;
    varShareClient<pgScanGUI::scanRes>* scanRes;

    //activation
    QHBoxLayout* alayout;
    hidCon* hcGoToNearestFree;
    QPushButton* btnGoToNearestFree;
    val_selector* selRadDilGoToNearestFree;
    val_selector* selRadSprGoToNearestFree;

    //settings
    QVBoxLayout* slayout;
private Q_SLOTS:
    bool goToNearestFree(); //returns true if failed (no free point nearby

};

#endif // CALIB_H
