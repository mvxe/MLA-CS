#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
class pgMoveGUI;

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis(pgMoveGUI* pgMGUI);
    rtoml::vsr conf;                //configuration map
    QWidget* gui_settings;

private:
    pgMoveGUI* pgMGUI;

    QVBoxLayout* slayout;
    val_selector* _writeBeamCenterOfsX;
    val_selector* _writeBeamCenterOfsY;
    QPushButton* btnReset;
    QPushButton* exOfsCalibBtn;

    double X_start, Y_start;

public:
    const std::atomic<double>& writeBeamCenterOfsX;     // in mm corrected
    const std::atomic<double>& writeBeamCenterOfsY;


private Q_SLOTS:
    void onExOfsCalibBtn(bool state);
};

#endif // BEAMANL_H
