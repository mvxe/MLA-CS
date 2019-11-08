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

    xMove=new eadScrlBar("Move X: ", 200,20,1.5);
    connect(xMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveX(double)));
    alayout->addWidget(xMove);
    yMove=new eadScrlBar("Move Y: ", 200,20,1.5);
    connect(yMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveY(double)));
    alayout->addWidget(yMove);
    zMove=new eadScrlBar("Move Z: ", 200,20,false,1.5);
    connect(zMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveZ(double)));
    alayout->addWidget(zMove);
    fMove=new eadScrlBar("Move F: ", 200,20,true,1.5);
    connect(fMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveF(double)));
    alayout->addWidget(fMove);
    FZdif=new val_selector(0, "F-Z= ", -200, 200, 6, 0, {"mm"});
    FZdif->setEnabled(false);
    connect(FZdif, SIGNAL(changed(double)), this, SLOT(_onMoveZF(double)));
    connect(fMove, SIGNAL(lock(bool)), this, SLOT(onLockF(bool)));
    alayout->addWidget(FZdif);
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
    QLabel* txt=new QLabel("(Click -> move X/Y -> focus -> Click)");
    tlay->addWidget(txt); tlay->addStretch(0); tlay->setMargin(0);
    slayout->addWidget(twid);
}

void pgMoveGUI::onMove(double Xmov, double Ymov, double Zmov, double Fmov){
    double _Xmov=Xmov*xMoveScale->val/1000;
    double _Ymov=Ymov*xMoveScale->val/1000;
    double _Zmov=Zmov*zMoveScale->val/1000+_Xmov*autoadjXZ->val+_Ymov*autoadjYZ->val;
    double _Fmov=Fmov*fMoveScale->val/1000-_Zmov;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,_Xmov,_Ymov,_Zmov,_Fmov);
}

void pgMoveGUI::onFZdifChange(double X, double Y, double Z, double F){
    if(FZdifCur==F+Z) return;
    ignoreNext=true;
    FZdif->setValue(F+Z);
    FZdifCur=F+Z;
}

void pgMoveGUI::_onMoveZF(double difference){
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
