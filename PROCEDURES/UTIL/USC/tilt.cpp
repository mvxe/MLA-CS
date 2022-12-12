#include "tilt.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgTiltGUI::pgTiltGUI(pgMoveGUI* pgMGUI): pgMGUI(pgMGUI){
    init_gui_activation();
    init_gui_settings();
}

void pgTiltGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    xTilt=new eadScrlBar("Adjust X Tilt: ", 250,20,true);
    connect(xTilt->abar, SIGNAL(change(double)), this, SLOT(_doTilt_X(double)));
    alayout->addWidget(xTilt);
    yTilt=new eadScrlBar("Adjust Y Tilt: ", 250,20,true);
    connect(yTilt->abar, SIGNAL(change(double)), this, SLOT(_doTilt_Y(double)));
    alayout->addWidget(yTilt);
}

void pgTiltGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    tilt_mult=new val_selector(1, "Tilt multiplier: ", -1000, 1000, 6);
    conf["tilt_mult"]=tilt_mult;
    slayout->addWidget(tilt_mult);
    focus_autoadjX=new val_selector(0, "Focus adjustment for X: ", -100, 100, 12);
    conf["focus_autoadjX"]=focus_autoadjX;
    slayout->addWidget(focus_autoadjX);
    focus_autoadjY=new val_selector(0, "Focus adjustment for Y: ", -100, 100, 12);
    conf["focus_autoadjY"]=focus_autoadjY;
    slayout->addWidget(focus_autoadjY);

    calib_focus_autoadjX=new QPushButton;
    calib_focus_autoadjX->setText("Calibrate X");
    calib_focus_autoadjX->setCheckable(true);
    connect(calib_focus_autoadjX, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_X(bool)));
    calib_focus_autoadjY=new QPushButton;
    calib_focus_autoadjY->setText("Calibrate Y");
    calib_focus_autoadjY->setCheckable(true);
    connect(calib_focus_autoadjY, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_Y(bool)));
    QLabel* txt=new QLabel("(Click -> tilt -> focus -> Click)");
    slayout->addWidget(new twid(calib_focus_autoadjX, calib_focus_autoadjY, txt));
}

void pgTiltGUI::doTilt(double magnitudeX, double magnitudeY, bool scale){
    go.pCTRL->motion("XTilt",tilt_mult->val*magnitudeX,0,0,CTRL::MF_RELATIVE);
    go.pCTRL->motion("YTilt",tilt_mult->val*magnitudeY,0,0,CTRL::MF_RELATIVE);
    pgMGUI->move(0,0,tilt_mult->val*(magnitudeX*focus_autoadjX->val+magnitudeY*focus_autoadjY->val));
}

void pgTiltGUI::onCalibrate(bool isStart, bool isX){
    if(isStart){
        if(isX) Tilt_cum_X=go.pCTRL->getMotionSetting("XTilt",CTRL::mst_position);
        else    Tilt_cum_Y=go.pCTRL->getMotionSetting("YTilt",CTRL::mst_position);
        pgMGUI->getPos(nullptr,nullptr,&Z_cum);
    }else{
        double tmp;
        pgMGUI->getPos(nullptr,nullptr,&tmp);
        Z_cum-=tmp;
        if(isX) focus_autoadjX->setValue(Z_cum/(go.pCTRL->getMotionSetting("XTilt",CTRL::mst_position)-Tilt_cum_X));
        else    focus_autoadjY->setValue(Z_cum/(go.pCTRL->getMotionSetting("YTilt",CTRL::mst_position)-Tilt_cum_Y));
    }
}
