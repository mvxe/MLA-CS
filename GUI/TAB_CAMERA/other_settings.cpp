#include "other_settings.h"
#include "GUI/gui_includes.h"
#include "includes.h"

cameraSett::cameraSett(std::atomic<bool>& getExpMinMax): getExpMinMax(getExpMinMax){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    measureFlag=new QCheckBox("Measure min,max in next scan.");
    layout->addWidget(measureFlag);
    layout->addWidget(new QLabel("[Excluded pixels will not be considered]"));
    report0=new QLabel();
    layout->addWidget(report0);
    layout->addWidget(new hline);
    expSel=new val_selector(1000, "Set Exposure: ", 0, 9999999, 3, 0, {"us"});
    conf["expSel"]=expSel;
    layout->addWidget(expSel);
    report1=new QLabel();
    layout->addWidget(report1);
    layout->addWidget(new hline);
    LEDon=new QCheckBox("LED toggle");
    LEDon->setToolTip("Just for testing, does not save/persist.");
    connect(LEDon, SIGNAL(toggled(bool)), this, SLOT(onLEDToggle(bool)));
    LEDon->setChecked(true);
    layout->addWidget(LEDon);

    genReport();
    connect(expSel, SIGNAL(changed()), this, SLOT(genReport()));
    connect(measureFlag, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
}

void cameraSett::genReport(){
    double FPSMax=-1;
    if(go.pGCAM->iuScope->connected){
        go.pGCAM->iuScope->set("ExposureTime",expSel->val);
        expSel->setValue(go.pGCAM->iuScope->get_dbl("ExposureTime"));
        go.pGCAM->iuScope->expo.set(expSel->val);
        double ignore;
        go.pGCAM->iuScope->get_frame_rate_bounds (&ignore, &FPSMax);
    }
    std::string rpt0=util::toString("Min pixel value: ",expMin,"\nMax pixel value: ",expMax,"\n");
    report0->setText(QString::fromStdString(rpt0));
    std::string rpt1=util::toString("Max FPS for selected exposure: ",FPSMax,"\n");
    report1->setText(QString::fromStdString(rpt1));
}
void cameraSett::onToggled(bool state){getExpMinMax=state;}
void cameraSett::doneExpMinmax(int min, int max){
    expMin=min;
    expMax=max;
    genReport();
}
void cameraSett::onLEDToggle(bool state){
    if(!go.pXPS->connected) return;
    go.pXPS->setGPIO(XPS::iuScopeLED,state);
}
double cameraSett::setExposurePreset(int N){
    if(N==0){
        go.pGCAM->iuScope->set("ExposureTime",expSel->val);
        return expSel->val;
    }else return -1;
}
