#include "calib.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/focus.h"
#include "PROCEDURES/UTIL/USC/move.h"
#include "PROCEDURES/UTIL/WRT/bounds.h"


pgCalib::pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv): pgSGUI(pgSGUI), pgBGUI(pgBGUI), pgFGUI(pgFGUI), pgMGUI(pgMGUI), pgDpEv(pgDpEv){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    btnGoToNearestFree=new QPushButton("Go to nearest free");
    btnGoToNearestFree->setToolTip("This adheres to write boundaries!");
    connect(btnGoToNearestFree, SIGNAL(released()), this, SLOT(goToNearestFree()));
    hcGoToNearestFree=new hidCon(btnGoToNearestFree);
    alayout->addWidget(hcGoToNearestFree);
    selRadDilGoToNearestFree=new val_selector(1, "pgCalib_selRadDilGoToNearestFree", "Exclusion Dilation Radius: ", 0, 100, 2, 0, {"um"});
    selRadSprGoToNearestFree=new val_selector(1, "pgCalib_selRadSprGoToNearestFree", "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
    hcGoToNearestFree->addWidget(selRadDilGoToNearestFree);
    hcGoToNearestFree->addWidget(selRadSprGoToNearestFree);
    btnSelectSaveFolder=new QPushButton("Select save folder");
    connect(btnSelectSaveFolder, SIGNAL(released()), this, SLOT(onSelSaveF()));
    alayout->addWidget(new twid(btnSelectSaveFolder));

    btnWriteCalibFocus=new QPushButton("Calibrate Write Focus");
    btnWriteCalibFocus->setEnabled(false);
    connect(btnWriteCalibFocus, SIGNAL(released()), this, SLOT(onWCF()));
    btnWriteCalibFocus->setCheckable(true);
    cbContWriteCalibFocus=new QCheckBox;
    cbContWriteCalibFocus->setText("Repeating scan");
    cbContWriteCalibFocus->setChecked(true);
    connect(cbContWriteCalibFocus, SIGNAL(toggled(bool)), this, SLOT(onWCFCont(bool)));
    alayout->addWidget(new twid(btnWriteCalibFocus, cbContWriteCalibFocus));

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    slayout->addWidget(new QLabel("Write Focus Calibration Measurements"));
    selWriteCalibFocusDoNMeas=new val_selector(100, "pgCalib_selWriteCalibFocusDoNMeas", "Do this many measurements: ", 1, 100000, 0);
    slayout->addWidget(selWriteCalibFocusDoNMeas);
    selWriteCalibFocusReFocusNth=new val_selector(5, "pgCalib_selWriteCalibFocusReFocusNth", "Refocus every this many measurements: ", 0, 1000, 0);
    selWriteCalibFocusReFocusNth->setToolTip("Set 0 to disable refocusing.");
    slayout->addWidget(selWriteCalibFocusReFocusNth);
    selWriteCalibFocusRadDil=new val_selector(1, "pgCalib_selWriteCalibFocusRadDil", "Exclusion Dilation Radius: ", 0, 100, 2, 0, {"um"});
    selWriteCalibFocusRadDil->setToolTip("This should be >3x your beam spot size...");
    slayout->addWidget(selWriteCalibFocusRadDil);
    selWriteCalibFocusRadSpr=new val_selector(1, "pgCalib_selWriteCalibFocusRadSpr", "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
    slayout->addWidget(selWriteCalibFocusRadSpr);
    selWriteCalibFocusThresh=new val_selector(0.2, "pgCalib_selWriteCalibFocusThresh", "2nd Derivative Exclusion Threshold: ", 0, 1, 4);
    selWriteCalibFocusThresh->setToolTip("Try out values in Depth Eval.");
    slayout->addWidget(selWriteCalibFocusThresh);
    selWriteCalibFocusRange=new val_selector(0, "pgCalib_selWriteCalibFocusRange", "Measurement range around focus: ", 0, 1000, 3, 0, {"um"});
    selWriteCalibFocusRange->setToolTip("Each measurement will be done at a random write beam focus around the starting focus\u00B1 this parameter.");
    slayout->addWidget(selWriteCalibFocusRange);
    selWriteCalibFocusMoveOOTW=new checkbox_save(false,"pgCalib_selWriteCalibFocusMoveOOTW","Take direct picture of the beam before each measurement.");
    slayout->addWidget(selWriteCalibFocusMoveOOTW);
    selWriteCalibFocusMoveOOTWDis=new val_selector(0, "pgCalib_selWriteCalibFocusMoveOOTWDis", "Move X this much to get out of the way: ", -1000, 1000, 3, 0, {"mm"});
    slayout->addWidget(selWriteCalibFocusMoveOOTWDis);

    scanRes=pgSGUI->result.getClient();
}
pgCalib::~pgCalib(){
    delete scanRes;
}


bool pgCalib::goToNearestFree(){
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //if there is a measurement in progress, wait till its done
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);       //first check if the current scan result is valid (done oncurrent position)
    bool redoScan=false;
    const pgScanGUI::scanRes* res=scanRes->get();
    if(res!=nullptr){
        for(int i=0;i!=3;i++) if(res->pos[i]!=tmp.pos[i])redoScan=true;
    }else redoScan=true;
    if(redoScan){
        pgSGUI->doOneRound();
        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        res=scanRes->get();
    }
    int dil=(selRadDilGoToNearestFree->val*1000/pgMGUI->getNmPPx()-0.5); if(dil<0) dil=0;
    cv::Mat mask=pgDpEv->getMaskFlatness(res, pgMGUI->getNmPPx(), dil);
    int ptX,ptY;
    imgAux::getNearestFreePointToCenter(&mask, ptX, ptY, selRadSprGoToNearestFree->val);    //make radius choosable in nm
    if(ptX==-1){
        std::cerr<<"No free nearby!\n";
        return true;
    }

    double dXumm, dYumm, dXmm, dYmm;
    dXumm=(ptX-res->depth.cols/2)*pgMGUI->getNmPPx()/1000000;
    dYumm=(ptY-res->depth.rows/2)*pgMGUI->getNmPPx()/1000000;
    dXmm=dXumm*cos(pgMGUI->getAngCamToXMot())+dYumm*sin(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());
    dYmm=dXumm*sin(pgMGUI->getAngCamToXMot())+dYumm*cos(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());

    pgMGUI->onMove(-dXmm,-dYmm,0,0);
    return false;
}
void pgCalib::onSelSaveF(){
    std::string temp=QFileDialog::getExistingDirectory(this, "Select Folder for Saving Calibration Data").toStdString();
    if(temp.empty()) return;
    saveFolderName=temp;
    btnWriteCalibFocus->setEnabled(true);
}
void pgCalib::onWCFCont(bool state){btnWriteCalibFocus->setCheckable(state);}
void pgCalib::onWCF(){
    if(btnWriteCalibFocus->isCheckable() && !btnWriteCalibFocus->isChecked()) return;

    if(btnWriteCalibFocus->isCheckable() && !(measCounter%((int)(selWriteCalibFocusReFocusNth->val)))){
        pgFGUI->refocus();
        while(!pgFGUI->focusingDone) QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
    }


    std::cerr<<"did one\n";

    if(btnWriteCalibFocus->isCheckable() && btnWriteCalibFocus->isChecked()){
        measCounter++;
        if(measCounter<(int)selWriteCalibFocusDoNMeas->val) onWCF();
        else {btnWriteCalibFocus->setChecked(false); measCounter=0;}
    } else measCounter=0;
}
