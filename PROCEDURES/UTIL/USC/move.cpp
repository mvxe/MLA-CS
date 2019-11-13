#include "move.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgMoveGUI::pgMoveGUI(){
    init_gui_activation();
    init_gui_settings();
}

void pgMoveGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);

    xMove=new eadScrlBar("Move X: ", 200,20);
    connect(xMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveX(double)));
    alayout->addWidget(xMove);
    yMove=new eadScrlBar("Move Y: ", 200,20);
    connect(yMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveY(double)));
    alayout->addWidget(yMove);
    zMove=new eadScrlBar("Move Z: ", 200,20,false);
    connect(zMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveZ(double)));
    alayout->addWidget(zMove);
    fMove=new eadScrlBar("Move F: ", 200,20,true);
    connect(fMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveF(double)));
    alayout->addWidget(fMove);

    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    FZdif=new val_selector(0, "F-Z= ", -200, 200, 6, 0, {"mm"});
    FZdif->setEnabled(false);
    connect(FZdif, SIGNAL(changed(double)), this, SLOT(_onMoveZF(double)));
    connect(fMove, SIGNAL(lock(bool)), this, SLOT(onLockF(bool)));
    tlay->addWidget(FZdif);
    mpow=new val_selector(0, "Move X10^", 0, 6, 0);
    tlay->addWidget(mpow);
    tlay->addStretch(0); tlay->setMargin(0);
    alayout->addWidget(twid);

    QWidget* twid2=new QWidget; QHBoxLayout* tlay2=new QHBoxLayout; twid2->setLayout(tlay2);
    addDial=new QPushButton;
    connect(addDial, SIGNAL(released()), this, SLOT(onAddDial()));
    addDial->setIcon(QPixmap(":/edit-add.svg"));
    addDial->setMaximumSize(20,20);
    tlay2->addWidget(addDial);
    rmDial=new QPushButton;
    connect(rmDial, SIGNAL(released()), this, SLOT(onRmDial()));
    rmDial->setIcon(QPixmap(":/gtk-no.svg"));
    rmDial->setMaximumSize(20,20);
    tlay2->addWidget(rmDial);
    tlay2->addWidget(new QLabel("Dis./Ang. Ctrl."));
    tlay2->addStretch(0); tlay2->setMargin(0);
    alayout->addWidget(twid2);
}

void pgMoveGUI::onLockF(bool locked){FZdif->setEnabled(!locked);}

void pgMoveGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    xMoveScale=new val_selector(0.001, "pgMoveGUI_xMoveScale", "X move multiplier: ", 0, 100, 6);
    slayout->addWidget(xMoveScale);
    yMoveScale=new val_selector(0.001, "pgMoveGUI_yMoveScale", "Y move multiplier: ", 0, 100, 6);
    slayout->addWidget(yMoveScale);
    zMoveScale=new val_selector(0.001, "pgMoveGUI_zMoveScale", "Z move multiplier: ", 0, 100, 6);
    slayout->addWidget(zMoveScale);
    fMoveScale=new val_selector(0.001, "pgMoveGUI_fMoveScale", "F move multiplier: ", 0, 100, 6);
    slayout->addWidget(fMoveScale);

    autoadjXZ=new val_selector(0, "pgMoveGUI_autoadjXZ", "Focus adjustment for X: ", -100, 100, 12);
    slayout->addWidget(autoadjXZ);
    autoadjYZ=new val_selector(0, "pgMoveGUI_autoadjYZ", "Focus adjustment for Y: ", -100, 100, 12);
    slayout->addWidget(autoadjYZ);
    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    calib_autoadjXZ=new QPushButton;
    calib_autoadjXZ->setText("Calibrate XZ");
    calib_autoadjXZ->setCheckable(true);
    connect(calib_autoadjXZ, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_X(bool)));
    tlay->addWidget(calib_autoadjXZ);
    calib_autoadjYZ=new QPushButton;
    calib_autoadjYZ->setText("Calibrate YZ");
    calib_autoadjYZ->setCheckable(true);
    connect(calib_autoadjYZ, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_Y(bool)));
    tlay->addWidget(calib_autoadjYZ);
    tlay->addWidget(new QLabel("(Click -> move X/Y -> focus -> Click)"));
    tlay->addStretch(0); tlay->setMargin(0);
    slayout->addWidget(twid);
}

void pgMoveGUI::_onMoveX(double magnitude){onMove(magnitude*xMoveScale->val/1000*pow(10,mpow->val),0,0,0);}
void pgMoveGUI::_onMoveY(double magnitude){onMove(0,magnitude*xMoveScale->val/1000*pow(10,mpow->val),0,0);}
void pgMoveGUI::_onMoveZ(double magnitude){onMove(0,0,magnitude*zMoveScale->val/1000*pow(10,mpow->val),0);}
void pgMoveGUI::_onMoveF(double magnitude){onMove(0,0,0,magnitude*fMoveScale->val/1000*pow(10,mpow->val));}
void pgMoveGUI::onMove(double Xmov, double Ymov, double Zmov, double Fmov){
    if(!go.pXPS->connected) return;
    double _Xmov=Xmov;
    double _Ymov=Ymov;
    double _Zmov=Zmov+_Xmov*autoadjXZ->val+_Ymov*autoadjYZ->val;
    double _Fmov=Fmov-_Zmov;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,_Xmov,_Ymov,_Zmov,_Fmov);
}

void pgMoveGUI::onFZdifChange(double X, double Y, double Z, double F){
    if(FZdifCur==F+Z) return;
    ignoreNext=true;
    FZdif->setValue(F+Z);
    FZdifCur=F+Z;
}

void pgMoveGUI::_onMoveZF(double difference){
    if(!go.pXPS->connected) return;
    if(ignoreNext){ignoreNext=false;return;}
    if(FZdifCur==-9999) return;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,0,difference-FZdifCur);
    FZdifCur=difference;
}

void pgMoveGUI::onCalibrate(bool isStart, bool isX){
    if(isStart){
        if(isX) X_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[0];
        else    Y_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[1];
        Z_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[2];
    }else{
        Z_cum=go.pXPS->getPos(XPS::mgroup_XYZF).pos[2]-Z_cum;
        if(isX) autoadjXZ->setValue(Z_cum/(go.pXPS->getPos(XPS::mgroup_XYZF).pos[0]-X_cum));
        else    autoadjYZ->setValue(Z_cum/(go.pXPS->getPos(XPS::mgroup_XYZF).pos[1]-Y_cum));
    }
}

void pgMoveGUI::onAddDial(){
    moveDial* moveFixed=new moveDial;
    alayout->insertWidget(alayout->count()-1,moveFixed);
    connect(moveFixed, SIGNAL(doMove(double,double)), this, SLOT(onDialMove(double,double)));
    moveDials.push_back(moveFixed);
}
void pgMoveGUI::onRmDial(){
    if(!moveDials.empty()){
        delete moveDials.back();
        moveDials.pop_back();
    }
}
void pgMoveGUI::onDialMove(double x,double y){onMove(-x/1000, -y/1000, 0, 0);}
