#include "tab_camera.h"
#include "GUI/mainwindow.h"
#include "includes.h"


tab_camera::tab_camera(QWidget* parent){
    layout=new QHBoxLayout;
    parent->setLayout(layout);

    LDisplay = new mtlabel;
    LDisplay->setMouseTracking(false);
    LDisplay->setFrameShape(QFrame::Box);
    LDisplay->setFrameShadow(QFrame::Plain);
    LDisplay->setScaledContents(false);
    LDisplay->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    TWCtrl = new QTabWidget;
    layout->addWidget(LDisplay);
    layout->addWidget(TWCtrl);
    TWCtrl->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

    pageMotion = new QWidget;
    pageWriting = new QWidget;
    pageSettings = new QWidget;
    TWCtrl->addTab(pageMotion,"Motion");

    TWCtrl->addTab(pageWriting,"Writing");

    TWCtrl->addTab(pageSettings,"Settings");
        layoutSettings=new QVBoxLayout;
        pageSettings->setLayout(layoutSettings);
        led_wl = new val_selector(470., "tab_camera_led_wl", "LED Wavelength:", 1., 2000., 0, {"nm"}, &changed);
        layoutSettings->addWidget(led_wl);
        coh_len = new val_selector(20., "tab_camera_coh_len", "Coherence Length L:", 1., 2000., 2, {"nm","um",QChar(0x03BB)}, &changed);
        layoutSettings->addWidget(coh_len);
        range = new val_selector(10., "tab_camera_range", "Scan Range:", 1., 2000., 3 , {"nm","um",QChar(0x03BB),"L"}, &changed);
        layoutSettings->addWidget(range);
        ppwl = new val_selector(20., "tab_camera_ppwl", "Points Per Wavelength: ", 4., 2000.,  &changed);
        layoutSettings->addWidget(ppwl);
        max_vel = new val_selector(300., "tab_camera_max_vel", "UScope stage max velocity: ", 1e-9, 300., 0, {"mm/s"}, &changed);
        layoutSettings->addWidget(max_vel);
        max_acc = new val_selector(2500., "tab_camera_max_acc", "UScope stage max acceleration: ", 1e-9, 2500., 0, {"mm/s^2"}, &changed);
        layoutSettings->addWidget(max_acc);
        calcL=new QLabel;
        layoutSettings->addWidget(calcL);
        layoutSettings->addStretch(0);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));
}

void tab_camera::work_fun(){
    //printf("working..\n");

    if(changed==true){
        changed=false;
        double min,max;
        go.pGCAM->iuScope->get_frame_rate_bounds (&min, &max);
        double time1=vsConv(range)*vsConv(ppwl)/vsConv(led_wl)/floor(max);
        double time2=2*sqrt(2*vsConv(range)/1000000/vsConv(max_acc));       //THIS IS WRONG TODO FIX
        calcL->setText(util::toString("Calculated single trip time: ",time1," s\nCalculated round trip time: ",2*time1+2*time2," s\n","Calculated velocity: ",vsConv(range)/1000000/time1," mm/s\n").c_str());     //unit is mm
    }

}

void tab_camera::tab_entered(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    running=true;
    go.pGCAM->iuScope->set_trigger("Line1");

    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);

    while(1){   //clear out leftover frames
        mat=framequeue->getUserMat();
        if(mat==nullptr) break;
        framequeue->freeUserMat();
    }
    timer->start(work_call_time);
}
void tab_camera::tab_exited(){
    if(running==false) return;
    running=false;
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
    go.pGCAM->iuScope->set_trigger("none");
    timer->stop();
}



double tab_camera::vsConv(val_selector* vs){
    switch(vs->index){
    case 0: return vs->val;
    case 1: return vs->val*1000;
    case 2: return vs->val*led_wl->val;
    case 3: return vs->val*coh_len->val*led_wl->val;
    }
}
