#include "move.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <dlib/optimization.h>

pgMoveGUI::pgMoveGUI(){
    init_gui_activation();
    init_gui_settings();
}

void pgMoveGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);

    xMove=new eadScrlBar("Move X: ", 200,20);
    connect(xMove->abar, SIGNAL(change(double)), this, SLOT(scaledMoveX(double)));
    alayout->addWidget(xMove);
    yMove=new eadScrlBar("Move Y: ", 200,20);
    connect(yMove->abar, SIGNAL(change(double)), this, SLOT(scaledMoveY(double)));
    alayout->addWidget(yMove);
    zMove=new eadScrlBar("Move Z: ", 200,20,false);
    connect(zMove->abar, SIGNAL(change(double)), this, SLOT(scaledMoveZ(double)));
    alayout->addWidget(zMove);
    fMove=new eadScrlBar("Move F: ", 200,20,true);
    connect(fMove->abar, SIGNAL(change(double)), this, SLOT(scaledMoveF(double)));
    alayout->addWidget(fMove);

    FZdif=new val_selector(0, "F-Z= ", -200, 200, 6, 0, {"mm"});
    FZdif->setEnabled(false);
    connect(FZdif, SIGNAL(changed(double)), this, SLOT(moveZF(double)));
    connect(fMove, SIGNAL(lock(bool)), this, SLOT(onLockF(bool)));
    mpow=new val_selector(0, "Move X10^", 0, 6, 0);
    alayout->addWidget(new twid(FZdif, mpow));

    addDial=new QPushButton;
    connect(addDial, SIGNAL(released()), this, SLOT(onAddDial()));
    addDial->setIcon(QPixmap(":/edit-add.svg"));
    addDial->setMaximumSize(20,20);
    rmDial=new QPushButton;
    connect(rmDial, SIGNAL(released()), this, SLOT(onRmDial()));
    rmDial->setIcon(QPixmap(":/gtk-no.svg"));
    rmDial->setMaximumSize(20,20);
    alayout->addWidget(new twid(addDial, rmDial, new QLabel("Dis./Ang. Ctrl.")));
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
    calib_autoadjXZ=new QPushButton;
    calib_autoadjXZ->setText("Calibrate XZ");
    calib_autoadjXZ->setCheckable(true);
    connect(calib_autoadjXZ, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_X(bool)));
    calib_autoadjYZ=new QPushButton;
    calib_autoadjYZ->setText("Calibrate YZ");
    calib_autoadjYZ->setCheckable(true);
    connect(calib_autoadjYZ, SIGNAL(toggled(bool)), this, SLOT(_onCalibrate_Y(bool)));
    slayout->addWidget(new twid(calib_autoadjXZ, calib_autoadjYZ));
    slayout->addWidget(new QLabel("(Click -> move X/Y -> focus -> Click)"));

    slayout->addWidget(new hline);

    markPointForCalib=new QPushButton("Mark Point");
    markPointForCalib->setCheckable(true);
    connect(markPointForCalib, SIGNAL(toggled(bool)), this, SLOT(onMarkPointForCalib(bool)));
    markPointForCalib->setToolTip("Make Sure the Center Mark is On Center (ie. Reset the Writing Beam Correction Offset).");
    ptFeedback=new QLabel("Have 0 points.");
    slayout->addWidget(new twid(markPointForCalib, ptFeedback));
    slayout->addWidget(new QLabel("Check^ -> click on distinct feature\n -> move and center on it -> Uncheck"));
    calculateCalib=new QPushButton("Calculate Calibration Constants"); calculateCalib->setToolTip("The more points, the better!");
    connect(calculateCalib, SIGNAL(released()), this, SLOT(onCalculateCalib()));
    slayout->addWidget(new twid(calculateCalib));
    calibNmPPx=new val_selector(10, "pgMoveGUI_XYnmppx", "XY calibration: ", 0, 1000, 6, 0, {"nm/px"});
    slayout->addWidget(calibNmPPx);
    calibAngCamToXMot=new val_selector(0, "pgMoveGUI_calibAngCamToXMot", "Camera/Xmot angle: ", -M_PI/2, M_PI/2, 6, 0, {"rad"});
    slayout->addWidget(calibAngCamToXMot);
    calibAngYMotToXMot=new val_selector(0, "pgMoveGUI_calibAngYMotToXMot", "Xmot/Ymot angle ofs: ", -M_PI/2, M_PI/2, 6, 0, {"rad"});
    slayout->addWidget(calibAngYMotToXMot);
}

void pgMoveGUI::scaledMoveX(double magnitude){move(magnitude*xMoveScale->val/1000*pow(10,mpow->val),0,0,0);}
void pgMoveGUI::scaledMoveY(double magnitude){move(0,magnitude*xMoveScale->val/1000*pow(10,mpow->val),0,0);}
void pgMoveGUI::scaledMoveZ(double magnitude){move(0,0,magnitude*zMoveScale->val/1000*pow(10,mpow->val),0);}
void pgMoveGUI::scaledMoveF(double magnitude){move(0,0,0,magnitude*fMoveScale->val/1000*pow(10,mpow->val));}
void pgMoveGUI::move(double Xmov, double Ymov, double Zmov, double Fmov){
    if(!go.pXPS->connected) return;
    double _Xmov=Xmov;
    double _Ymov=Ymov;
    double _Zmov=Zmov+_Xmov*autoadjXZ->val+_Ymov*autoadjYZ->val;
    double _Fmov=Fmov-_Zmov;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,_Xmov,_Ymov,_Zmov,_Fmov);
}

void pgMoveGUI::onFZdifChange(double X, double Y, double Z, double F){
    if(FZdifCur==F+Z) return;
    FZdif->setValue(F+Z);
}

void pgMoveGUI::moveZF(double difference){
    if(!go.pXPS->connected) return;
    if(difference==FZdifCur || FZdifCur==-9999) return;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,0,difference-FZdifCur);
    FZdifCur=difference;
    FZdif->setValue(FZdifCur);
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
void pgMoveGUI::onDialMove(double x,double y){move(-x/1000, -y/1000, 0, 0);}



void pgMoveGUI::delvrNextClickPixDiff(double Dx, double Dy){
    reqstNextClickPixDiff=false;
    curP4calib.DXpx=Dx;
    curP4calib.DYpx=Dy;
}
void pgMoveGUI::onMarkPointForCalib(bool state){
    if(state){
        reqstNextClickPixDiff=true;
        XPS::raxis temp=go.pXPS->getPos(XPS::mgroup_XYZF);
        curP4calib.DXmm=-temp.pos[0];
        curP4calib.DYmm=-temp.pos[1];
    }else{
        if(reqstNextClickPixDiff==true) return;
        XPS::raxis temp=go.pXPS->getPos(XPS::mgroup_XYZF);
        curP4calib.DXmm+=temp.pos[0];
        curP4calib.DYmm+=temp.pos[1];
        p4calib.push_back(curP4calib);
        ptFeedback->setText(QString::fromStdString(util::toString("Have ",p4calib.size()," points.")));
    }
}


typedef dlib::matrix<double,4,1> input_vector;
typedef dlib::matrix<double,3,1> parameter_vector;
double residual(const std::pair<input_vector, double>& data, const parameter_vector& params){
    double DXpx=data.first(0);  double mmPPx=params(0);
    double DYpx=data.first(1);  double phi0=params(1);
    double DXmm=data.first(2);  double phi1=params(2);
    double DYmm=data.first(3);
    double model=abs(DXmm*cos(phi0)+DYmm*sin(phi0+phi1)-mmPPx*DXpx)+abs(DXmm*sin(phi0)+DYmm*cos(phi0+phi1)-mmPPx*DYpx);
    return model*model - data.second;
}

void pgMoveGUI::onCalculateCalib(){
    std::vector<std::pair<input_vector, double>> data;
    while(!p4calib.empty()){
        std::cout<<"points: "<<p4calib.back().DXpx<<" "<<p4calib.back().DYpx<<" "<<p4calib.back().DXmm<<" "<<p4calib.back().DYmm<<"\n";
        input_vector input{p4calib.back().DXpx,p4calib.back().DYpx,p4calib.back().DXmm,p4calib.back().DYmm};
        data.push_back(std::make_pair(input, 0));
        p4calib.pop_back();
    }
    ptFeedback->setText("Have 0 points.");
    parameter_vector res;
    res=1;
    dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-12).be_verbose(), residual, derivative(residual), data, res);
    std::cout << "inferred parameters: "<< dlib::trans(res) << "\n";
    if(res(0)<0){
        res(0)*=-1;
        res(1)*=-1;
    }
    while(res(0)<=-M_PI/2) res(0)+=M_PI;
    while(res(0) > M_PI/2) res(0)-=M_PI;
    while(res(1)<=-M_PI/2) res(1)+=M_PI;
    while(res(1) > M_PI/2) res(1)-=M_PI;
    calibNmPPx->setValue(res(0)*1000000);
    calibAngCamToXMot->setValue(-res(1));
    calibAngYMotToXMot->setValue(-res(2));
}
double pgMoveGUI::getNmPPx(){return calibNmPPx->val;}
double pgMoveGUI::getAngCamToXMot(){return calibAngCamToXMot->val;}
double pgMoveGUI::getYMotToXMot(){return calibAngYMotToXMot->val;}
