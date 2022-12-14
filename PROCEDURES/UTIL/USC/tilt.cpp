#include "tilt.h"
#include "GUI/gui_includes.h"

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

    tiltPreset=new smp_selector("Apply tilt preset: ",0,{"none"});
    connect(tiltPreset, SIGNAL(changed()), this, SLOT(onTiltPreset()));
    connect(tiltPreset, SIGNAL(aboutToShow()), this, SLOT(onUpdateMenu()));
    conf["vec_pnames"]=vec_pnames;
    conf["vec_pxtilt"]=vec_pxtilt;
    conf["vec_pytilt"]=vec_pytilt;
    tiltPresetAdd=new QPushButton("");
    tiltPresetAdd->setMaximumSize(20,20);
    tiltPresetAdd->setIcon(QPixmap(":/edit-add.svg"));
    tiltPresetRemove=new QPushButton("");
    tiltPresetRemove->setMaximumSize(20,20);
    tiltPresetRemove->setIcon(QPixmap(":/gtk-no.svg"));
    alayout->addWidget(new twid(tiltPreset,tiltPresetAdd,tiltPresetRemove));
    connect(tiltPresetAdd, SIGNAL(released()), this, SLOT(onTiltPresetAdd()));
    connect(tiltPresetRemove, SIGNAL(released()), this, SLOT(onTiltPresetRemove()));
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

void pgTiltGUI::onTiltPresetAdd(){
    QString text=QInputDialog::getText(nullptr, tr("Add preset with current tilt value"),tr("Specify name for new tilt preset:"));
    if (!text.isEmpty()){
        vec_pnames.push_back(text.toStdString());
        vec_pxtilt.push_back(go.pCTRL->getMotionSetting("XTilt",CTRL::mst_position));
        vec_pytilt.push_back(go.pCTRL->getMotionSetting("YTilt",CTRL::mst_position));
        std::vector<QString> qvec_pnames;
        qvec_pnames.push_back("none");
        for(auto& item:vec_pnames) qvec_pnames.push_back(QString::fromStdString(item));
        tiltPreset->replaceLabels(vec_pnames.size(),qvec_pnames);
    }
}
void pgTiltGUI::onTiltPresetRemove(){
    std::string id=tiltPreset->get();
    for(size_t i=0;i!=vec_pnames.size();i++) if(vec_pnames[i]==id){
        vec_pnames.erase(vec_pnames.begin()+i);
        vec_pxtilt.erase(vec_pxtilt.begin()+i);
        vec_pytilt.erase(vec_pytilt.begin()+i);
        std::vector<QString> qvec_pnames;
        qvec_pnames.push_back("none");
        for(auto& item:vec_pnames) qvec_pnames.push_back(QString::fromStdString(item));
        tiltPreset->replaceLabels(0,qvec_pnames);
        break;
    }
}

void pgTiltGUI::onTiltPreset(){
    std::string id=tiltPreset->get();
    for(size_t i=0;i!=vec_pnames.size();i++) if(vec_pnames[i]==id){
        go.pCTRL->motion("XTilt",vec_pxtilt[i],0,0);
        go.pCTRL->motion("YTilt",vec_pytilt[i],0,0);
        break;
    }
}

void pgTiltGUI::onUpdateMenu(){
    std::vector<QString> qvec_pnames;
    qvec_pnames.push_back("none");
    for(size_t i=0;i!=vec_pnames.size();i++){
        qvec_pnames.push_back(QString::fromStdString(vec_pnames[i]));
    }
    tiltPreset->replaceLabels(0,qvec_pnames);
}
