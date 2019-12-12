#ifndef BEAMANL_H
#define BEAMANL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"

class pgBeamAnalysis: public QWidget{
    Q_OBJECT
public:
    pgBeamAnalysis();
    QWidget* gui_activation;

private:
    QVBoxLayout* alayout;
    QPushButton* btnGetCenter;

private Q_SLOTS:
    void getWritingBeamCenter();
};

#endif // BEAMANL_H
