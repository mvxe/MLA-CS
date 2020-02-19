#include "correction.h"
#include "GUI/gui_includes.h"
#include "includes.h"

const std::string pgCorrection::fileName{"_calib.pfm"};
pgCorrection::pgCorrection(pgScanGUI* pgSGUI, pgMoveGUI* pgMGUI): pgSGUI(pgSGUI), pgMGUI(pgMGUI){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    selArrayXsize=new val_selector(10, "pgCorrection_selArrayXsize", "Array Size X:", 1, 1000, 0);
    slayout->addWidget(selArrayXsize);
    selArrayYsize=new val_selector(10, "pgCorrection_selArrayYsize", "Array Size Y:", 1, 1000, 0);
    slayout->addWidget(selArrayYsize);
    selArraySpacing=new val_selector(5, "pgCorrection_selArraySpacing", "Array Spacing:", 0.001, 100, 3, 0, {"um"});
    slayout->addWidget(selArraySpacing);
    selAvgNum=new val_selector(10, "pgCorrection_selAvgNum", "Number of measurements per grid point:", 1, 1000, 0);
    slayout->addWidget(selAvgNum);
    selExclusionSD=new val_selector(2, "pgCorrection_selExclusionSD", "Exlude is dif>this*SD:", 0.1, 100, 2);
    slayout->addWidget(selExclusionSD);
    corBlur=new val_selector(0, util::toString("pgCorrection_corBlur"), "Correction Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
    slayout->addWidget(corBlur);
    preserveResult=new QCheckBox("Preserve result (for fiddling with SD).");
    slayout->addWidget(preserveResult);
    startCalib=new QPushButton("Start Calibration");
    startCalib->setCheckable(true);
    recalcCalib=new QPushButton("Recalculate");
    recalcCalib->setEnabled(false);
    slayout->addWidget(new twid(startCalib,recalcCalib));
    connect(startCalib, SIGNAL(toggled(bool)), this, SLOT(onStartCalib(bool)));
    connect(preserveResult, SIGNAL(toggled(bool)), this, SLOT(onToggle(bool)));
    connect(recalcCalib, SIGNAL(released()), this, SLOT(onStartRecalc()));
    scanResC=pgSGUI->result.getClient();
    setAsCalib=new QPushButton("Use This Calibration");
    setAsCalib->setEnabled(false);
    connect(setAsCalib, SIGNAL(released()), this, SLOT(onSetAsCalib()));
    slayout->addWidget(new twid(setAsCalib));
    slayout->addWidget(new hline);
    QLabel* lbl=new QLabel(util::toString("The correction file is saved as ",fileName,". It can be replaced by the button above, but you may also manually save a scan over it and restart the program.").c_str());
    lbl->setWordWrap(true);
    slayout->addWidget(lbl);
    enableCorrection=new checkbox_save(false,"pgCorrection_enableCorrection","Enable correction.");
    connect(enableCorrection, SIGNAL(changed(bool)), this, SLOT(enableCorrectionChanged(bool)));
    slayout->addWidget(enableCorrection);
    cor=new pgScanGUI::scanRes;
    if(!pgSGUI->loadScan(cor, fileName)) {cor=nullptr; std::cerr<<"Could not find pgCorrection file: "<<fileName<<"\n";useCorr.lock();useCorrLocked=true;}
}
pgCorrection::~pgCorrection(){
    if(scans!=nullptr) delete scans;
    delete scanResC;
}
void pgCorrection::onSetAsCalib(){
    if(!useCorrLocked) {useCorr.lock();useCorrLocked=true;}
    pgSGUI->saveScan(&avg, fileName);
    if(cor!=nullptr) delete cor;
    cor=new pgScanGUI::scanRes();
    *cor=avg;
    if(enableCorrection->val){useCorr.unlock();useCorrLocked=false;}
}
void pgCorrection::enableCorrectionChanged(bool state){
    if(state){
        if(cor!=nullptr) {useCorr.unlock();useCorrLocked=false;}
    }else{
        if(!useCorrLocked) {useCorr.lock();useCorrLocked=true;}
    }
}

void pgCorrection::onStartCalib(bool state){
    if(!state) return;
    if(scans!=nullptr) {recalcCalib->setEnabled(false); delete scans; scans=nullptr;}
    scans=new std::vector<pgScanGUI::scanRes>;

    double xOfs=((selArrayXsize->val-1)*selArraySpacing->val)/2000;         //in mm
    double yOfs=((selArrayYsize->val-1)*selArraySpacing->val)/2000;
    pgMGUI->move(xOfs,yOfs,0,0);
    for(int j=0;j<selArrayYsize->val; j++){
        for(int i=0;i<selArrayXsize->val; i++){
            if(!startCalib->isChecked()){   //abort
                std::cerr<<"Aborting calibration.\n";
                return;
            }
            while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait for motion to complete
            if(!useCorrLocked)useCorr.lock();   //disabling correction temporarily
            pgSGUI->doNRounds((int)selAvgNum->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(0,0,0,0), -1);
            if(!useCorrLocked)useCorr.unlock();
            scans->emplace_back(*scanResC->get());

            std::cerr<<"\nDone "<<1+i+j*selArrayXsize->val<<"/"<<selArrayYsize->val*selArrayXsize->val<<"\n\n";
            if(i<selArrayXsize->val-1) pgMGUI->move(-selArraySpacing->val/1000,0,0,0);
        }
        if(j<selArrayYsize->val-1) pgMGUI->move(2*xOfs,-selArraySpacing->val/1000,0,0);
    }
    pgMGUI->move(xOfs,yOfs,0,0);
    onStartRecalc();
    //pgScanGUI::saveScan(&avg);
    startCalib->setChecked(false);
    if(!preserveResult->isChecked()) {recalcCalib->setEnabled(false); delete scans; scans=nullptr;}
    else recalcCalib->setEnabled(true);
    setAsCalib->setEnabled(true);
}
void pgCorrection::onStartRecalc(){
    if(scans==nullptr) return;
    avg=pgSGUI->avgScans(scans, selExclusionSD->val);
    if(corBlur->val>0){
        cv::Mat blured;
        cv::bilateralFilter(avg.depth, blured, -1, corBlur->val, corBlur->val);
        blured.copyTo(avg.depth);
        cv::bilateralFilter(avg.depthSS, blured, -1, corBlur->val, corBlur->val);
        blured.copyTo(avg.depthSS);
    }
    Q_EMIT sendToDisplay(avg);
}
void pgCorrection::onToggle(bool state){
    if(state==false && scans!=nullptr) {recalcCalib->setEnabled(false); delete scans; scans=nullptr;}
}
