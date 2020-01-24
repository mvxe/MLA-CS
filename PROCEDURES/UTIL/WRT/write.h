#ifndef PGWRITE_H
#define PGWRITE_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"

class pgWrite: public QObject{
    Q_OBJECT
public:
    pgWrite();
    QWidget* gui_activation;
    QWidget* gui_settings;

private:
    //activation
    QVBoxLayout* alayout;
    QPushButton* pulse;
    val_selector* pulseInt;
    val_selector* pulseDur;


    //settings
    QVBoxLayout* slayout;
private Q_SLOTS:
    void onPulse();
};

#endif // PGWRITE_H
