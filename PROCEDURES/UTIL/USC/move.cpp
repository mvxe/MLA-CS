#include "move.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <dlib/optimization.h>

pgMoveGUI::pgMoveGUI(smp_selector* _selObjective): selObjective(_selObjective){
    init_gui_activation();
    init_gui_settings();
    connect(selObjective, SIGNAL(changed(int)), this, SLOT(_chooseObj(int)));
    connect(this, SIGNAL(sigChooseObj(bool)), this, SLOT(chooseObj(bool)));
    conf["selObjective"]=selObjective;
    conf["objectiveDisplacement-X"]=objectiveDisplacement[0];
    conf["objectiveDisplacement-Y"]=objectiveDisplacement[1];
    conf["objectiveDisplacement-Z"]=objectiveDisplacement[2];
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

    mpow=new val_selector(0, "Move X10^", 0, 6, 0);
    alayout->addWidget(new twid(mpow));

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

void pgMoveGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    xMoveScale=new val_selector(0.001, "X move multiplier: ", 0, 100, 6);
    conf["xMoveScale"]=xMoveScale;
    slayout->addWidget(xMoveScale);
    yMoveScale=new val_selector(0.001, "Y move multiplier: ", 0, 100, 6);
    conf["yMoveScale"]=yMoveScale;
    slayout->addWidget(yMoveScale);
    zMoveScale=new val_selector(0.001, "Z move multiplier: ", 0, 100, 6);
    conf["zMoveScale"]=zMoveScale;
    slayout->addWidget(zMoveScale);

    autoadjXZ=new val_selector(0, "Z adjustment for X: ", -100, 100, 12);
    conf["autoadjXZ"]=autoadjXZ;
    slayout->addWidget(autoadjXZ);
    autoadjYZ=new val_selector(0, "Z adjustment for Y: ", -100, 100, 12);
    conf["autoadjYZ"]=autoadjYZ;
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
    slayout->addWidget(new QLabel("(Click -> move X/Y -> focus -> Click) (make sure to first disable Correct XY skew)"));

    slayout->addWidget(new hline);

    markObjDis=new QPushButton("Mark");
    markObjDis->setCheckable(true);
    connect(markObjDis, SIGNAL(toggled(bool)), this, SLOT(_onMarkObjDisY(bool)));
    slayout->addWidget(new twid(new QLabel("Mirau-Writing objective move calibration"), markObjDis));
    slayout->addWidget(new QLabel("(Focus and center on feature with Mirau objective -> Check^\n-> focus and center on feature with Writing objective -> Uncheck)"));

    slayout->addWidget(new hline);

    markPointForCalib=new QPushButton("Mark Point");
    markPointForCalib->setCheckable(true);
    connect(markPointForCalib, SIGNAL(toggled(bool)), this, SLOT(onMarkPointForCalib(bool)));
    markPointForCalib->setToolTip("Make Sure the Center Mark is On Center (ie. Reset the Writing Beam Correction Offset).");
    ptFeedback=new QLabel("Have 0 points.");
    slayout->addWidget(new twid(markPointForCalib, ptFeedback));
    slayout->addWidget(new QLabel("(Check^ -> click on distinct feature -> move and center on it -> Uncheck)"));
    calculateCalib=new QPushButton("Calculate Calibration Constants"); calculateCalib->setToolTip("The more points, the better (min 5)!");
    connect(calculateCalib, SIGNAL(released()), this, SLOT(onCalculateCalib()));
    slayout->addWidget(new twid(calculateCalib));

    settingsObjective=new smp_selector("Settings for objective:", 0, {"Mirau","Writing"});
    lastindex=0;
    connect(settingsObjective, SIGNAL(changed(int)), this, SLOT(onSettingsObjectiveChange(int)));
    slayout->addWidget(new twid(settingsObjective));
    calibMirauW=new QWidget;
    calibMirauL=new QVBoxLayout;
    calibMirauW->setLayout(calibMirauL);
    slayout->addWidget(calibMirauW);
    calibWritingW=new QWidget;
    calibWritingL=new QVBoxLayout;
    calibWritingW->setLayout(calibWritingL);
    slayout->addWidget(calibWritingW);
    calibWritingW->setVisible(false);

        calibNmPPx=new val_selector(10, "XY calibration: ", 0, 1000, 6, 0, {"nm/px"});
        conf["calibNmPPx"]=calibNmPPx;
        calibMirauL->addWidget(calibNmPPx);
        calibAngCamToXMot=new val_selector(0, "Camera/Xmot angle: ", -M_PI, M_PI, 6, 0, {"rad"});
        conf["calibAngCamToXMot"]=calibAngCamToXMot;
        connect(calibAngCamToXMot, SIGNAL(changed()), this, SLOT(reCalcConstM()));
        calibMirauL->addWidget(calibAngCamToXMot);
        calibMirauL->addWidget(new QLabel("NOTE: these settings are used for most moves, including procedures."));

        calibNmPPx_writing=new val_selector(10, "XY calibration: ", 0, 1000, 6, 0, {"nm/px"});
        conf["calibNmPPx_writing"]=calibNmPPx_writing;
        calibWritingL->addWidget(calibNmPPx_writing);
        calibAngCamToXMot_writing=new val_selector(0, "Camera/Xmot angle: ", -M_PI, M_PI, 6, 0, {"rad"});
        connect(calibAngCamToXMot_writing, SIGNAL(changed()), this, SLOT(reCalcConstW()));
        conf["calibAngCamToXMot_writing"]=calibAngCamToXMot_writing;
        calibWritingL->addWidget(calibAngCamToXMot_writing);
        calibWritingL->addWidget(new QLabel("NOTE: these settings are used only for Writing objective GUI click moves."));

    calibAngYMotToXMot=new val_selector(M_PI/2, "Xmot/Ymot angle ofs: ", -M_PI, M_PI, 6, 0, {"rad"});
    calibAngYMotToXMot->setToolTip("This setting is calibrated with Mirau objective, and should be independent of objective.");
    conf["calibAngYMotToXMot"]=calibAngYMotToXMot;
    connect(calibAngYMotToXMot, SIGNAL(changed()), this, SLOT(reCalcConstM()));
    connect(calibAngYMotToXMot, SIGNAL(changed()), this, SLOT(reCalcConstW()));
    slayout->addWidget(calibAngYMotToXMot);
    disableSkewCorrection=new checkbox_gs(true,"Disable skew correction.");
    conf["disableSkewCorrection"]=disableSkewCorrection;
    slayout->addWidget(disableSkewCorrection);
}

void pgMoveGUI::scaledMoveX(double magnitude){move(magnitude*xMoveScale->val/1000*pow(10,mpow->val),0,0);}
void pgMoveGUI::scaledMoveY(double magnitude){move(0,magnitude*xMoveScale->val/1000*pow(10,mpow->val),0);}
void pgMoveGUI::scaledMoveZ(double magnitude){move(0,0,magnitude*zMoveScale->val/1000*pow(10,mpow->val));}
void pgMoveGUI::move(double Xmov, double Ymov, double Zmov){
    double _Xmov=Xmov;
    double _Ymov=Ymov;
    if(!disableSkewCorrection->val){
        _Xmov=Xraw(Xmov,Ymov,0);
        _Ymov=Yraw(Xmov,Ymov,0);
    }

    double _Zmov=Zmov+_Xmov*autoadjXZ->val+_Ymov*autoadjYZ->val;    //this correction should change with skewCorrection, but implementing that would be annoying (dont want to add more configuration) and its negligible anyway, just recalibrate autoadjXZ,YZ with skewCorrection if its a problem
    go.pRPTY->motion("X",_Xmov,0,0,CTRL::MF_RELATIVE);
    go.pRPTY->motion("Y",_Ymov,0,0,CTRL::MF_RELATIVE);
    go.pRPTY->motion("Z",_Zmov,0,0,CTRL::MF_RELATIVE);
}

void pgMoveGUI::corCOMove(CTRL::CO& co, double Xmov, double Ymov, double Zmov, bool forceSkewCorrection){
    double _Xmov=Xmov;
    double _Ymov=Ymov;
    if(!disableSkewCorrection->val || forceSkewCorrection){
        _Xmov=Xraw(Xmov,Ymov,0);
        _Ymov=Yraw(Xmov,Ymov,0);
    }
    double _Zmov=Zmov+_Xmov*autoadjXZ->val+_Ymov*autoadjYZ->val;
    co.addMotion("X",_Xmov,0,0,CTRL::MF_RELATIVE);
    co.addMotion("Y",_Ymov,0,0,CTRL::MF_RELATIVE);
    co.addMotion("Z",_Zmov,0,0,CTRL::MF_RELATIVE);
}

void pgMoveGUI::_chooseObj(int index){
    Q_EMIT sigChooseObjExpo(index==0);
    if(!markObjDis->isChecked()) markObjDis->setEnabled(index==0);
    if(currentObjective!=0 && currentObjective!=1){     // we ignore first signal (default -1) - likely to be configuration read
        currentObjective=index;
        return;
    }else if(currentObjective==index) return;
    bool useMirau=(index==0);
    if(!go.pRPTY->connected){
        Q_EMIT sigChooseObj(!useMirau);  // revert menu
        return;
    }
    currentObjective=index;
    go.pRPTY->motion("X",(useMirau?-1:1)*objectiveDisplacement[0],0,0,CTRL::MF_RELATIVE);
    go.pRPTY->motion("Y",(useMirau?-1:1)*objectiveDisplacement[1],0,0,CTRL::MF_RELATIVE);
    go.pRPTY->motion("Z",(useMirau?-1:1)*objectiveDisplacement[2],0,0,CTRL::MF_RELATIVE);
}
void pgMoveGUI::chooseObj(bool useMirau){
    if(currentObjective==(useMirau?0:1)) return;
    selObjective->set(useMirau?"Mirau":"Writing");
    while(currentObjective!=(useMirau?0:1)) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);   //wait till switch is done
}

void pgMoveGUI::onCalibrate(bool isStart, bool isX){
    if(isStart){
        if(isX) X_cum=go.pRPTY->getMotionSetting("X",CTRL::mst_position);
        else    Y_cum=go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
        Z_cum=go.pRPTY->getMotionSetting("Z",CTRL::mst_position);
    }else{
        Z_cum=go.pRPTY->getMotionSetting("Z",CTRL::mst_position)-Z_cum;
        if(isX) autoadjXZ->setValue(Z_cum/(go.pRPTY->getMotionSetting("X",CTRL::mst_position)-X_cum));
        else    autoadjYZ->setValue(Z_cum/(go.pRPTY->getMotionSetting("Y",CTRL::mst_position)-Y_cum));
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
void pgMoveGUI::onDialMove(double x,double y){move(x/1000, y/1000, 0);}



void pgMoveGUI::delvrNextClickPixDiff(double Dx, double Dy){
    reqstNextClickPixDiff=false;
    curP4calib.DXpx=Dx;
    curP4calib.DYpx=Dy;
}
void pgMoveGUI::onMarkPointForCalib(bool state){
    if(currentObjective!=settingsObjective->index){
        QMessageBox::critical(gui_settings, "Cannot mark point", "Objective to be calibrated != current objective. Change objective before starting to calibrate.");
        markPointForCalib->setChecked(false);
        return;
    }
    if(state){
        reqstNextClickPixDiff=true;
        curP4calib.DXmm=-go.pRPTY->getMotionSetting("X",CTRL::mst_position);
        curP4calib.DYmm=-go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
    }else{
        if(reqstNextClickPixDiff==true) return;
        curP4calib.DXmm+=go.pRPTY->getMotionSetting("X",CTRL::mst_position);
        curP4calib.DYmm+=go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
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
    double model=(abs(DXpx*cos(phi0)+DYpx*sin(phi0)-DXmm/abs(mmPPx))+abs(DXpx*cos(phi1+phi0)+DYpx*sin(phi1)*cos(phi0)-DYmm/abs(mmPPx)))*abs(mmPPx);
    return model*model - data.second;
}

void pgMoveGUI::onCalculateCalib(){
    if(p4calib.size()<5){
        QMessageBox::critical(gui_settings, "Calibration aborted", "Fewer than 5 points selected! Select more points.");
        return;
    }
    std::vector<std::pair<input_vector, double>> data;
    while(!p4calib.empty()){
        std::cout<<"points: "<<p4calib.back().DXpx<<" "<<p4calib.back().DYpx<<" "<<p4calib.back().DXmm<<" "<<p4calib.back().DYmm<<"\n";
        input_vector input{p4calib.back().DXpx,p4calib.back().DYpx,p4calib.back().DXmm,p4calib.back().DYmm};
        data.push_back(std::make_pair(input, 0));
        p4calib.pop_back();
    }
    ptFeedback->setText("Have 0 points.");
    parameter_vector res;
    res={0.001,0.001,M_PI};
    dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-12).be_verbose(), residual, derivative(residual), data, res);
    std::cout << "inferred parameters: "<< dlib::trans(res) << "\n";
    if(res(0)<0)
        res(0)*=-1;
    while(res(1)<=-M_PI) res(1)+=2*M_PI;
    while(res(1) > M_PI) res(1)-=2*M_PI;
    while(res(2)<=-M_PI) res(2)+=2*M_PI;
    while(res(2) > M_PI) res(2)-=2*M_PI;

    if(settingsObjective->index==0){    // Mirau
        calibNmPPx->setValue(res(0)*1000000);
        calibAngCamToXMot->setValue(res(1));
        calibAngYMotToXMot->setValue(res(2));
    }else{  // Writing
        calibNmPPx_writing->setValue(res(0)*1000000);
        calibAngCamToXMot_writing->setValue(res(1));
    }
}
double pgMoveGUI::getNmPPx(int index){
    if(index==0 || (index!=1 && currentObjective==0)) return calibNmPPx->val;
    else return calibNmPPx_writing->val;
}
double pgMoveGUI::getAngCamToXMot(int index){
    if(index==0 || (index!=1 && currentObjective==0)) return calibAngCamToXMot->val;
    else return calibAngCamToXMot_writing->val;
}
double pgMoveGUI::getYMotToXMot(){return calibAngYMotToXMot->val;}

void pgMoveGUI::reCalcConst(bool isMirau){
    a[!isMirau]=cos(getAngCamToXMot(!isMirau));
    b[!isMirau]=sin(getAngCamToXMot(!isMirau));
    c[!isMirau]=cos(getAngCamToXMot(!isMirau)+getYMotToXMot());
    d[!isMirau]=sin(getYMotToXMot())*cos(getAngCamToXMot(!isMirau));
    A[!isMirau]=d[!isMirau]/(a[!isMirau]*d[!isMirau]-b[!isMirau]*c[!isMirau]);
    B[!isMirau]=b[!isMirau]/(b[!isMirau]*c[!isMirau]-a[!isMirau]*d[!isMirau]);
    C[!isMirau]=(1.-a[!isMirau]*A[!isMirau])/b[!isMirau];
    D[!isMirau]=-a[!isMirau]*B[!isMirau]/b[!isMirau];
}
double pgMoveGUI::Xraw(double Xcor, double Ycor, int index){
    if(index!=1) index=0;   // default to mirau (for index=-1)
    return a[index]*Xcor+b[index]*Ycor;
}
double pgMoveGUI::Yraw(double Xcor, double Ycor, int index){
    if(index!=1) index=0;
    return c[index]*Xcor+d[index]*Ycor;
}
double pgMoveGUI::Xcor(double Xraw, double Yraw, int index){
    if(index!=1) index=0;
    return A[index]*Xraw+B[index]*Yraw;
}
double pgMoveGUI::Ycor(double Xraw, double Yraw, int index){
    if(index!=1) index=0;
    return C[index]*Xraw+D[index]*Yraw;
}
double pgMoveGUI::mm2px(double coord, int index, double nmppx){
    if(nmppx==0) nmppx=getNmPPx(index);
    return coord/nmppx*1000000;
}
double pgMoveGUI::px2mm(double coord, int index, double nmppx){
    if(nmppx==0) nmppx=getNmPPx(index);
    return coord*nmppx/1000000;
}

void pgMoveGUI::getPos(double* X, double* Y, double* Z){
    double xr, yr;
    xr=go.pRPTY->getMotionSetting("X",CTRL::mst_position);
    yr=go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
    *X=Xcor(xr-((currentObj==1)?objectiveDisplacement[0]:0),yr-((currentObj==1)?objectiveDisplacement[1]:0),0);
    if(Y!=nullptr) *Y=Ycor(xr-((currentObj==1)?objectiveDisplacement[0]:0),yr-((currentObj==1)?objectiveDisplacement[1]:0),0);
    if(Z!=nullptr) *Z=go.pRPTY->getMotionSetting("Z",CTRL::mst_position);
}

void pgMoveGUI::_onMarkObjDisY(bool isStart){
    if(isStart){
        tmpOD[0]=go.pRPTY->getMotionSetting("X",CTRL::mst_position);
        tmpOD[1]=go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
        tmpOD[2]=go.pRPTY->getMotionSetting("Z",CTRL::mst_position);
    }else{
        for(unsigned i=0;i!=3;i++) objectiveDisplacement[i]=0;  // to prevent move
        chooseObj(false);
        objectiveDisplacement[0]=go.pRPTY->getMotionSetting("X",CTRL::mst_position)-tmpOD[0];
        objectiveDisplacement[1]=go.pRPTY->getMotionSetting("Y",CTRL::mst_position)-tmpOD[1];
        objectiveDisplacement[2]=go.pRPTY->getMotionSetting("Z",CTRL::mst_position)-tmpOD[2];
    }
}
void pgMoveGUI::onSettingsObjectiveChange(int index){
    if(lastindex!=index){
        lastindex=index;
        p4calib.clear();
        markPointForCalib->setChecked(false);
        ptFeedback->setText(QString::fromStdString(util::toString("Have 0 points.")));
        calibWritingW->setVisible(index!=0);
        calibMirauW->setVisible(index==0);
    }
}
