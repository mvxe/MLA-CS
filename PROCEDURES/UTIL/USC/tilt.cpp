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
    xTilt=new eadScrlBar("Adjust X Tilt: ", 200,20);
    connect(xTilt->abar, SIGNAL(change(double)), this, SLOT(_work_fun_T(double)));
    alayout->addWidget(xTilt);
    yTilt=new eadScrlBar("Adjust Y Tilt: ", 200,20);
    connect(yTilt->abar, SIGNAL(change(double)), this, SLOT(_work_fun_F(double)));
    alayout->addWidget(yTilt);
}

void pgTiltGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    tilt_mult=new val_selector(0, "pgTiltGUI_tilt_mult", "Tilt multiplier: ", -100, 100, 6);
    slayout->addWidget(tilt_mult);
    focus_autoadj=new val_selector(0, "pgTiltGUI_focus_autoadj", "Focus adjustment for X: ", -100, 100, 12);
    slayout->addWidget(focus_autoadj);

    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    calib_focus_autoadj=new QPushButton;
    calib_focus_autoadj->setText("Calibrate (Click -> tilt -> focus -> Click)");
    calib_focus_autoadj->setCheckable(true);
    connect(calib_focus_autoadj, SIGNAL(toggled(bool)), this, SLOT(onCalibrate(bool)));
    tlay->addWidget(calib_focus_autoadj); tlay->addStretch(0); tlay->setMargin(0);
    slayout->addWidget(twid);

    tilt_motor_speed=new val_selector(100, "pgTiltGUI_tilt_motor_speed", "Tilt motor speed: ", 0, 5000, 0);
    slayout->addWidget(tilt_motor_speed);
}

void pgTiltGUI::work_fun(double magnitude, bool isX){
    if(!go.pCNC->connected || !go.pXPS->connected) return;
    if(!inited){
        go.pCNC->execCommand("M999\n");
        std::this_thread::sleep_for (std::chrono::milliseconds(10));
        go.pCNC->execCommand("M18 S10\n"); //set stepper disable inactivity to 10 seconds
        go.pCNC->execCommand("G91\n");      //set to relative positioning
        inited=true;
    }
    double dXY,dZ,vel;
    dXY=magnitude*tilt_mult->val;
    vel=tilt_motor_speed->val;
    if(isX) go.pCNC->execCommand("G1 X",dXY," F",vel,"\n");
    else    go.pCNC->execCommand("G1 Y",dXY," F",vel,"\n");
    dZ=dXY*focus_autoadj->val;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,dZ,-dZ);
    Tilt_cum+=dXY;
}

void pgTiltGUI::onCalibrate(bool isStart){
    if(isStart){
        Tilt_cum=0;
        Z_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[3];
    }else{
        Z_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[3]-Z_cum;
        focus_autoadj->setValue(-Z_cum/Tilt_cum);
    }
}
