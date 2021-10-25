#include "beamanl.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgBeamAnalysis::pgBeamAnalysis(pgMoveGUI* pgMGUI): pgMGUI(pgMGUI),
    _writeBeamCenterOfsX(new val_selector(0, "Center X Offset: ",-1,1,5,0,{"mm"})),
    writeBeamCenterOfsX{_writeBeamCenterOfsX->val},
    _writeBeamCenterOfsY(new val_selector(0, "Center Y Offset: ",-1,1,5,0,{"mm"})),
    writeBeamCenterOfsY{_writeBeamCenterOfsY->val}
    {

    conf["writeBeamCenterOfsX"]=_writeBeamCenterOfsX;
    conf["writeBeamCenterOfsY"]=_writeBeamCenterOfsY;

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    slayout->addWidget(_writeBeamCenterOfsX);
    slayout->addWidget(_writeBeamCenterOfsY);
    exOfsCalibBtn=new QPushButton;
    exOfsCalibBtn->setText("Get Offset");
    exOfsCalibBtn->setCheckable(true);
    connect(exOfsCalibBtn, SIGNAL(toggled(bool)), this, SLOT(onExOfsCalibBtn(bool)));
    slayout->addWidget(new twid(exOfsCalibBtn));
    slayout->addWidget(new QLabel("Extra Offset calib: ( Click^ -> write point -> move to point -> Click )"));
}

void pgBeamAnalysis::onExOfsCalibBtn(bool state){
    double X,Y;
    pgMGUI->getPos(&X,&Y);
    if(state){
        X_start=X;
        Y_start=Y;
    }else{
        _writeBeamCenterOfsX->setValue(_writeBeamCenterOfsX->val+X_start-X);
        _writeBeamCenterOfsY->setValue(_writeBeamCenterOfsY->val+Y_start-Y);
        pgMGUI->move(X_start-X, Y_start-Y, 0);
    }
}
