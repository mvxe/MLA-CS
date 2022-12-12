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
    guiCamFPS=new val_selector(30, "Set camera fps for GUI: ", 1, 240, 0);
    conf["guiCamFPS"]=guiCamFPS;
    layout->addWidget(guiCamFPS);
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
    layout->addWidget(new hline);
    layout->addWidget(new QLabel("CLAHE (only Writing Objective) settings (enable via checkbox)"));
    CLAHE_tileGridSize=new val_selector(10, "Set titleGridSize (symmetric X==Y): ", 2, 100, 0);
    conf["CLAHE_tileGridSize"]=CLAHE_tileGridSize;
    layout->addWidget(CLAHE_tileGridSize);
    CLAHE_clipLimit=new val_selector(2, "Set clipLimit: ", 0, 100, 2);
    conf["CLAHE_clipLimit"]=CLAHE_clipLimit;
    layout->addWidget(CLAHE_clipLimit);
    layout->addWidget(new hline);
    wrsbar_unit=new val_selector(0, "Writing obj. Scalebar Unit: ", -1000, 1000, 2, 0, {"um"});
    conf["wrsbar_unit"]=wrsbar_unit;
    layout->addWidget(wrsbar_unit);

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
    go.pCTRL->setGPIO("ilumLED", state);
}
void cameraSett::changeObjective(bool __isMirau){
    _isMirau=__isMirau;
    if(!go.pGCAM->iuScope->connected) return;
    if(_isMirau) go.pGCAM->iuScope->set("ExposureTime",expMirau->val);
    else        go.pGCAM->iuScope->set("ExposureTime",expWriting->val);
}
