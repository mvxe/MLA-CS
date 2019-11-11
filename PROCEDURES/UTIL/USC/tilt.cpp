#include "tilt.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgTiltGUI::pgTiltGUI(){
    init_gui_activation();
    init_gui_settings();
}

void pgTiltGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    xTilt=new eadScrlBar("Adjust X Tilt: ", 200,20,true);
    connect(xTilt->abar, SIGNAL(change(double)), this, SLOT(_doTilt_X(double)));
    alayout->addWidget(xTilt);
    yTilt=new eadScrlBar("Adjust Y Tilt: ", 200,20,true);
    connect(yTilt->abar, SIGNAL(change(double)), this, SLOT(_doTilt_Y(double)));
    alayout->addWidget(yTilt);
}

void pgTiltGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    tilt_mult=new val_selector(0, "pgTiltGUI_tilt_mult", "Tilt multiplier: ", -100, 100, 6);
    slayout->addWidget(tilt_mult);
    focus_autoadjX=new val_selector(0, "pgTiltGUI_focus_autoadjX", "Focus adjustment for X: ", -100, 100, 12);
    slayout->addWidget(focus_autoadjX);
    focus_autoadjY=new val_selector(0, "pgTiltGUI_focus_autoadjY", "Focus adjustment for Y: ", -100, 100, 12);
    slayout->addWidget(focus_autoadjY);

    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    calib_focus_autoadjX=new QPushButton;
    calib_focus_autoadjX->setText("Calibrate X");
    calib_focus_autoadjX->setCheckable(true);
    connect(calib_focus_autoadjX, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_X(bool)));
    tlay->addWidget(calib_focus_autoadjX);
    calib_focus_autoadjY=new QPushButton;
    calib_focus_autoadjY->setText("Calibrate Y");
    calib_focus_autoadjY->setCheckable(true);
    connect(calib_focus_autoadjY, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_Y(bool)));
    tlay->addWidget(calib_focus_autoadjY);
    QLabel* txt=new QLabel("(Click -> tilt -> focus -> Click)");
    tlay->addWidget(txt); tlay->addStretch(0); tlay->setMargin(0);
    slayout->addWidget(twid);

    tilt_motor_speed=new val_selector(100, "pgTiltGUI_tilt_motor_speed", "Tilt motor speed: ", 0, 5000, 0);
    slayout->addWidget(tilt_motor_speed);
}

void pgTiltGUI::doTilt(double magnitudeX, double magnitudeY, bool scale){
    if(!go.pCNC->connected || !go.pXPS->connected) return;
    if(!inited){
        go.pCNC->execCommand("M80\n");      //turn on PSU (this actually just inits the stepper drivers for now)
        go.pCNC->execCommand("M18 S10\n");  //set stepper disable inactivity to 10 seconds
        go.pCNC->execCommand("G91\n");      //set to relative positioning
        inited=true;
    }
    double dX, dY, dZ, vel; dZ=0;
    if(scale) {
        dX=magnitudeX*tilt_mult->val;
        dY=magnitudeY*tilt_mult->val;
    }
    else{
        dX=magnitudeX;
        dY=magnitudeY;
    }
    vel=tilt_motor_speed->val;
    if(magnitudeX!=0 && magnitudeY!=0) go.pCNC->execCommand("G1 X",dX," Y",dY," F",vel,"\n");
    else if(magnitudeX!=0)             go.pCNC->execCommand("G1 X",dX," F",vel,"\n");
    else if(magnitudeY!=0)             go.pCNC->execCommand("G1 Y",dY," F",vel,"\n");
    if(magnitudeX!=0) dZ+=dX*focus_autoadjX->val;
    if(magnitudeY!=0) dZ+=dY*focus_autoadjY->val;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,dZ,-dZ);
    if(magnitudeX!=0) Tilt_cum_X+=dX;
    if(magnitudeY!=0) Tilt_cum_Y+=dY;
}

void pgTiltGUI::onCalibrate(bool isStart, bool isX){
    if(isStart){
        if(isX) Tilt_cum_X=0;
        else    Tilt_cum_Y=0;
        Z_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[2];
    }else{
        Z_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[2]-Z_cum;
        if(isX) focus_autoadjX->setValue(Z_cum/Tilt_cum_X);
        else    focus_autoadjY->setValue(Z_cum/Tilt_cum_Y);
    }
}
