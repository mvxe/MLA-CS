#include "other_settings.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/move.h"

cameraSett::cameraSett(std::atomic<bool>& getExpMinMax, pgMoveGUI* pgMov): getExpMinMax(getExpMinMax){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    measureFlag=new QCheckBox("Measure min,max in next scan.");
    layout->addWidget(measureFlag);
    layout->addWidget(new QLabel("[Excluded pixels will not be considered]"));
    report0=new QLabel();
    layout->addWidget(report0);
    layout->addWidget(new hline);
    expMirau=new val_selector(1000, "Set exposure for Mirau: ", 0, 9999999, 3, 0, {"us"});
    conf["expMirau"]=expMirau;
    layout->addWidget(expMirau);
    expWriting=new val_selector(1000, "Set exposure for Writing: ", 0, 9999999, 3, 0, {"us"});
    conf["expWriting"]=expWriting;
    layout->addWidget(expWriting);
    layout->addWidget(new hline);
    LEDon=new QCheckBox("LED toggle");
    LEDon->setToolTip("Just for testing, does not save/persist.");
    connect(LEDon, SIGNAL(toggled(bool)), this, SLOT(onLEDToggle(bool)));
    LEDon->setChecked(true);
    layout->addWidget(LEDon);

    connect(expMirau, SIGNAL(changed()), this, SLOT(_changeObjectiveMirau()));
    connect(expWriting, SIGNAL(changed()), this, SLOT(_changeObjectiveWriting()));
    connect(measureFlag, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
    connect(pgMov, SIGNAL(sigChooseObjExpo(bool)), this, SLOT(changeObjective(bool)));
}

void cameraSett::genReport(){
    if(!go.pGCAM->iuScope->connected) return;

    std::string rpt0=util::toString("Min pixel value: ",expMin,"\nMax pixel value: ",expMax,"\n");
    report0->setText(QString::fromStdString(rpt0));
}
void cameraSett::onToggled(bool state){getExpMinMax=state;}
void cameraSett::doneExpMinmax(int min, int max){
    expMin=min;
    expMax=max;
    genReport();
}
void cameraSett::onLEDToggle(bool state){
    if(!go.pRPTY->connected) return;
    go.pRPTY->setGPIO("ilumLED", state);
}
void cameraSett::changeObjective(bool _isMirau){
    isMirau=_isMirau;
    if(!go.pGCAM->iuScope->connected) return;
    if(isMirau) go.pGCAM->iuScope->set("ExposureTime",expMirau->val);
    else        go.pGCAM->iuScope->set("ExposureTime",expWriting->val);
}
