#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <opencv2/phase_unwrapping.hpp>
#include <opencv2/core/ocl.hpp>
#include "GUI/TAB_CAMERA/colormap.h"

pScan::pScan(){

}
pScan::~pScan(){

}
void pScan::run(){

}



//GUI

pgScanGUI::pgScanGUI(procLockProg& MLP, cv::Rect& sROI): MLP(MLP), sROI(sROI){
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
    timerCM = new QTimer(this);
    timerCM->setInterval(timerCM_delay);
    timerCM->setSingleShot(true);
    connect(timerCM, SIGNAL(timeout()), this, SLOT(onBScan()));
    connect(this, SIGNAL(signalQMessageBoxWarning(QString, QString)), this, SLOT(slotQMessageBoxWarning(QString, QString)));
}

pgScanGUI::~pgScanGUI(){
    if(COmeasure!=nullptr) delete COmeasure;
}

void pgScanGUI::slotQMessageBoxWarning(QString title, QString text){
    QMessageBox::critical(gui_activation, title, text);
}

void pgScanGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    bScan=new QPushButton;
    bScan->setText("Scan");
    connect(bScan, SIGNAL(released()), this, SLOT(onBScan()));
    bScanContinuous=new QCheckBox;
    bScanContinuous->setText("Repeat");
    connect(bScanContinuous, SIGNAL(toggled(bool)), this, SLOT(onBScanContinuous(bool)));
    tiltCorrection=new smp_selector("Tilt correction:", 0, {"disabled","per scan", "enabled"});
    connect(tiltCorrection, SIGNAL(changed(int)), this, SLOT(onTiltCorrection(int)));
    conf["tiltCorrection"]=tiltCorrection;
    conf["tiltCorrectionX"]=tiltCor[0];
    conf["tiltCorrectionY"]=tiltCor[1];
    tiltScan=new QPushButton;
    tiltScan->setText("Tilt Scan");
    tiltScan->setToolTip("Do this scan on a uniform portion of the flat sample. Repeat every time tilt changes (ie a flat sample is exchanged).");
    tiltScan->setVisible(false);
    connect(tiltScan, SIGNAL(released()), this, SLOT(onBTiltScan()));

    cbAvg=new checkbox_gs(false,"Average");
    conf["cbAvg"]=cbAvg;
    cbGetRefl=new checkbox_gs(false,"Reflectivity");
    conf["cbGetRefl"]=cbGetRefl;
    alayout->addWidget(new twid(bScan, bScanContinuous, cbAvg, cbGetRefl));
    alayout->addWidget(new twid(tiltCorrection, tiltScan));

    xDifShift=new val_selector(0, "", -1000, 1000, 1, 0, {"px"});
    yDifShift=new val_selector(0, "", -1000, 1000, 1, 0, {"px"});
    gui_processing=new twid(new QLabel("dif. Shift (x,y): "),xDifShift,yDifShift);
    gui_processing->setToolTip("This applies to all functions that used dif. scans!");
}
void pgScanGUI::onBScanContinuous(bool status){bScan->setCheckable(status);}
void pgScanGUI::onBScan(){
    if(bScan->isCheckable() && !bScan->isChecked());
    else if(MLP._lock_proc.try_lock()){MLP._lock_proc.unlock();doOneRound();}
    if(bScan->isCheckable() && bScan->isChecked()) timerCM->start();
}
void pgScanGUI::onBTiltScan(){
    getTiltCalibOnNextScan=true;
    if(MLP._lock_proc.try_lock()){MLP._lock_proc.unlock();doOneRound();}
}
void pgScanGUI::onTiltCorrection(int index){
    tiltScan->setVisible(index==2);
}
void pgScanGUI::onBScanCW(){
    if(MLP._lock_proc.try_lock()){getWl=true; MLP._lock_proc.unlock();doOneRound();}
}
void pgScanGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    led_wl=new val_selector(470., "LED Wavelength:", 0.001, 2000., 2, 0, {"nm","um"});
    led_wl->setToolTip("The LED central wavelength. NOTE: the wavelength of the measured sine is actually half this wavelength.");
    conf["led_wl"]=led_wl;
    connect(led_wl, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(led_wl);
    scanWl=new QPushButton;
    scanWl->setText("Correct Wavelength (performs scan!)");
    connect(scanWl, SIGNAL(released()), this, SLOT(onBScanCW()));
    scanWlNxNpixels=new val_selector(10, "NxN, Select N", 1, 100, 0);
    conf["scanWlNxNpixels"]=scanWlNxNpixels;
    slayout->addWidget(new twid(scanWl,scanWlNxNpixels));
    coh_len=new val_selector(20., "Coherence Length L:", 1., 2000., 2, 2, {"nm","um",QChar(0x03BB)});
    conf["coh_len"]=coh_len;
    connect(coh_len, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(coh_len);
    direction=new checkbox_gs(false,"Flip Scan Direction");
    direction->setToolTip("Change this if you measure inverted depth.");
    conf["scanDirectionFlip"]=direction;
    connect(direction, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(direction);
    selectScanSetting=new smp_selector("Select scan setting: ", 0, {"Standard", "Precise", "Set2", "Set3", "Set4"});    //should have Nset strings
    conf["selectScanSetting"]=selectScanSetting;
    slayout->addWidget(selectScanSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new scanSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    slayout->addWidget(new hline());
    hideavg=new hidCon(new QLabel("Averaging"));
    hidedbg=new hidCon(new QLabel("Debug"));
    slayout->addWidget(hideavg);
    slayout->addWidget(hidedbg);
    connect(selectScanSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    avgDiscardCriteria=new val_selector(5.0, "Averaging Mask% Discard Threshold:", 0.1, 100., 1, 1, {"%"});
    conf["avgDiscardCriteria"]=avgDiscardCriteria;
    avgDiscardCriteria->setToolTip("If the difference in the number of excluded element between the current measurement and the avg measurement (which combines all previous exclusions(or)) is larger than this percentage times the total num of pixels, the measurement is discarded.");
    hideavg->addWidget(avgDiscardCriteria);
    connect(avgDiscardCriteria, SIGNAL(changed()), this, SLOT(recalculate()));
    avgEnableAntiDrift=new checkbox_gs(false,"Enable Anti-Drift for averaging (TODO; broken)");
    avgEnableAntiDrift->setToolTip("This enables a subpixel drift correction with correlation and shift.");
    conf["avgEnableAntiDrift"]=avgEnableAntiDrift;
    hideavg->addWidget(avgEnableAntiDrift);
    avgAntiDriftStep=new val_selector(0.001, "Drift correction step:", 0.001, 1., 3, 0, {"px"});
    conf["avgAntiDriftStep"]=avgAntiDriftStep;
    hideavg->addWidget(avgAntiDriftStep);
    triggerAdditionalDelay=new val_selector(0., "Additional trigger delay:", 0., 1000., 2, 1, {"ms"});
    conf["triggerAdditionalDelay"]=triggerAdditionalDelay;
    triggerAdditionalDelay->setToolTip("If scan drops frames, it might be that the returned max FPS is incorrect. Use this parameter to compensate (try 1 ms).");
    connect(triggerAdditionalDelay, SIGNAL(changed()), this, SLOT(recalculate()));
    hidedbg->addWidget(triggerAdditionalDelay);
    saveAvgMess=new QPushButton("Autosave raw measurements"); saveAvgMess->setCheckable(true);
    saveAvgMess->setToolTip("Select folder for saving all subsequent individual raw measurements. Last ROI applies (ie you must try saving something raw by selection first), else whole image is saved. Click again to disable further saving.");
    hidedbg->addWidget(new twid(saveAvgMess));
    saveNextMirrorBaselineHist=new QPushButton("Save next mirror baseline histogram.");
    hidedbg->addWidget(new twid(saveNextMirrorBaselineHist));
    connect(saveAvgMess, SIGNAL(released()), this, SLOT(onBSaveAvgMess()));
    connect(saveNextMirrorBaselineHist, SIGNAL(released()), this, SLOT(onBSaveNextMirrorBaselineHist()));
    onMenuChange(0);
}
void pgScanGUI::scanRes::copyTo(scanRes& dst) const{
    dst.max=max; dst.min=min;
    dst.pos[0]=pos[0]; dst.XYnmppx=XYnmppx;
    dst.pos[1]=pos[1]; dst.pos[2]=pos[2];
    dst.tiltCor[0]=tiltCor[0]; dst.tiltCor[1]=tiltCor[1];
    dst.avgNum=avgNum;
    depth.copyTo(dst.depth);
    mask.copyTo(dst.mask);
    maskN.copyTo(dst.maskN);
    depthSS.copyTo(dst.depthSS);
    refl.copyTo(dst.refl);
}

scanSettings::scanSettings(uint num, pgScanGUI* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    DFTrange=new val_selector(10., "DFT Range:", 1, 100, 0, 0 , {QChar(0x03BB)+QString{"/2"}});
    parent->conf[parent->selectScanSetting->getLabel(num)]["DFTrange"]=DFTrange;
    connect(DFTrange, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(DFTrange);
    range=new val_selector(0, "(Optional, longer ranges) Scan Range:", 0.0, 2000., 5, 3 , {"nm","um",QChar(0x03BB),"L"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["range"]=range;
    connect(range, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(range);
    pphwl=new val_selector(20., "Points Per Period (= half wavelength): ", 6, 2000., 2);
    parent->conf[parent->selectScanSetting->getLabel(num)]["pphwl"]=pphwl;
    connect(pphwl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(pphwl);
    dis_thresh=new val_selector(255./16, "Peak Discard Threshold (1/8 of signal amplitude): ", 0.01, 255./8, 2);
    parent->conf[parent->selectScanSetting->getLabel(num)]["dis_thresh"]=dis_thresh;
    slayout->addWidget(dis_thresh);
    unwrap=new checkbox_gs(true,"Unwrap.");
    unwrap->setToolTip("Long scans are reduced to [0:2pi) for the opencv unwrap function and potential info is lost");
    parent->conf[parent->selectScanSetting->getLabel(num)]["unwrap"]=unwrap;
    connect(unwrap, SIGNAL(changed()), parent, SLOT(rstAvg()));
    slayout->addWidget(unwrap);
    calcL=new QLabel;
    slayout->addWidget(calcL);
    exclDill=new val_selector(0, "Excluded dillation: ", 0, 100, 0, 0, {"px"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["exclDill"]=exclDill;
    slayout->addWidget(exclDill);
    tiltCorBlur=new val_selector(2, "Tilt Correction Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["tiltCorBlur"]=tiltCorBlur;
    slayout->addWidget(tiltCorBlur);
    tiltCorThrs=new val_selector(0.2, "Tilt Correction 2nd Derivative Exclusion Threshold: ", 0, 1, 2);
    parent->conf[parent->selectScanSetting->getLabel(num)]["tiltCorThrs_"]=tiltCorThrs;
    slayout->addWidget(tiltCorThrs);
    findBaseline=new checkbox_gs(false,"Correct for mirror baseline (used only with per scan tilt cor.).");
    parent->conf[parent->selectScanSetting->getLabel(num)]["findBaseline"]=findBaseline;
    slayout->addWidget(findBaseline);
    findBaselineHistStep=new val_selector(0.1, "Mirror baseline cor. hist. step: ", 0.01, 10, 2, 0, {"nm"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["findBaselineHistStep"]=findBaselineHistStep;
    slayout->addWidget(findBaselineHistStep);
}
void pgScanGUI::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    DFTrange=settingWdg[index]->DFTrange;
    range=settingWdg[index]->range;
    pphwl=settingWdg[index]->pphwl;
    dis_thresh=settingWdg[index]->dis_thresh;
    unwrap=settingWdg[index]->unwrap;
    calcL=settingWdg[index]->calcL;
    exclDill=settingWdg[index]->exclDill;
    tiltCorBlur=settingWdg[index]->tiltCorBlur;
    tiltCorThrs=settingWdg[index]->tiltCorThrs;
    findBaseline=settingWdg[index]->findBaseline;
    findBaselineHistStep=settingWdg[index]->findBaselineHistStep;
    recalculate();
}

void pgScanGUI::recalculate() {
    Q_EMIT recalculateCOs();
    if(MLP._lock_proc.try_lock()){
        if(MLP._lock_comp.try_lock()){
            std::string report;
            updateCO(report);
            calcL->setText(report.c_str());
            MLP._lock_comp.unlock();
        }
        else timer->start();
        MLP._lock_proc.unlock();
    }
    else timer->start();
}
void pgScanGUI::rstAvg(){
    skipAvgSettingsChanged=true;
}

void pgScanGUI::updateCO(std::string &report){
    if(!go.pGCAM->iuScope->connected || !go.pRPTY->connected) return;
    CORdy=false;
    if(COmeasure==nullptr){
        COmeasure=new CTRL::CO(go.pRPTY);
    }else if(COmeasure->_ctrl!=go.pRPTY){
        delete COmeasure;
        COmeasure=new CTRL::CO(go.pRPTY);
    }else COmeasure->clear();


    double minFPS, expo;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &COfps);
    expo=go.pGCAM->iuScope->expo*0.000001; // in s

    double totalScanRange=DFTrange->val*led_wl->val/2;    // in nm
    if(vsConv(range)>totalScanRange){
        totalScanRange=vsConv(range);
        report+=util::toString("Doing wider scan, total scan range set to Scan range.\n");
    }else report+=util::toString("Total scan range set to DFT range.\n");

    double readRangeDis=totalScanRange/1000000;  // in mm
    report+=util::toString("Total scan range = ",readRangeDis," mm.\n");
    double velocity=go.pRPTY->getMotionSetting("Z",CTRL::mst_defaultVelocity);
    double acceleration=go.pRPTY->getMotionSetting("Z",CTRL::mst_defaultAcceleration);
    displacementOneFrame=vsConv(led_wl)/2/vsConv(pphwl)/1000000;       // in mm
    report+=util::toString("One frame displacement = ",displacementOneFrame*1000000," nm.\n");
    totalFrameNum=std::round(readRangeDis/displacementOneFrame);   // for simplicity no image taken on one edge
    expectedDFTFrameNum=std::round(DFTrange->val*vsConv(pphwl));
    double timeOneFrame=1./COfps;
    double motionTimeOneFrame;
    bool hasConstantVelocitySegment;
    mcutil::evalAccMotion(displacementOneFrame, acceleration, velocity, &motionTimeOneFrame, &hasConstantVelocitySegment);

    report+=util::toString("Total number of frames = ",totalFrameNum,".\n");
    report+=util::toString("Expected number of frames for DFT = ",expectedDFTFrameNum,".\n");
    report+=util::toString("One frame time = ",motionTimeOneFrame+expo," s (ideal minimum), as limited by motion time.\n");
    report+=util::toString("One frame time = ",timeOneFrame," s, as limited by framerate.\n");
    report+=util::toString("One frame movement has ",hasConstantVelocitySegment?"a":"no"," constant speed component.\n");

    double timeToEdge;  // the time it takes to move from center(initial position) to edge(scan starting position)
    mcutil::evalAccMotion(readRangeDis/2, acceleration, velocity, &timeToEdge, &hasConstantVelocitySegment);
    report+=util::toString("To scan edge movement time = ",timeToEdge," s.\n");
    report+=util::toString("Total read time (if limited by framerate) = ",2*timeToEdge+timeOneFrame*totalFrameNum," s.\n");

    peakLoc=expectedDFTFrameNum/vsConv(pphwl);
    report+=util::toString("Expecting the peak to be at i = ", peakLoc," in the FFT.\n");

    COmeasure->addMotion("Z",(direction->val?1:-1)*readRangeDis/2,0,0,CTRL::MF_RELATIVE);    // move from center to -edge
    COmeasure->addHold("Z",CTRL::he_motion_ontarget);
    for(unsigned i=0;i!=totalFrameNum;i++){
        COmeasure->startTimer("timer",1./COfps+triggerAdditionalDelay->val*0.001);          // in case motion is completed before the camera is ready for another frame
        COmeasure->pulseGPIO("trigCam",0.0001);             // 100us
        if(expo>0.0001) COmeasure->addDelay(expo-0.0001);   // keep stationary during exposure
        COmeasure->addMotion("Z",(direction->val?-1:1)*displacementOneFrame,0,0,CTRL::MF_RELATIVE);
        COmeasure->addHold("Z",CTRL::he_motion_ontarget);
        COmeasure->addHold("timer",CTRL::he_timer_done);
    }
    COmeasure->addMotion("Z",(direction->val?1:-1)*readRangeDis/2,0,0,CTRL::MF_RELATIVE);    // move from +edge to center

    CORdy=true;
    skipAvgSettingsChanged=true;
}
int NA=0;
void pgScanGUI::doOneRound(cv::Rect ROI, char cbAvg_override, bool force_disable_tilt_correction, char refl_override){
    if(!CORdy) recalculate();
    measurementInProgress=true;
    go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound, this, ROI,cbAvg_override, force_disable_tilt_correction, refl_override));
}

void pgScanGUI::doNRounds(int N, cv::Rect ROI, double redoIfMaskHasMore, int redoN, bool force_disable_tilt_correction, char refl_override){
    if(!CORdy) recalculate();
    varShareClient<pgScanGUI::scanRes>* scanRes=result.getClient();

    int doneN=0;
    const pgScanGUI::scanRes* res;
    do{
        measurementInProgress=true;
        go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound, this, ROI, -1,force_disable_tilt_correction, refl_override));
        while(measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        res=scanRes->get();
        doneN++;
    }while(cv::countNonZero(res->mask)>res->mask.cols*res->mask.rows*redoIfMaskHasMore && doneN<=redoN);

    while(res->avgNum<N){
        if(MLP._lock_proc.try_lock()){
            go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound,this, ROI, 1,force_disable_tilt_correction, refl_override));
            MLP._lock_proc.unlock();
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        res=scanRes->get();
    }
    delete scanRes;
    while(!MLP._lock_proc.try_lock()) {QCoreApplication::processEvents(QEventLoop::AllEvents, 10); std::this_thread::sleep_for(std::chrono::milliseconds(100));}
    MLP._lock_proc.unlock();
    while(!MLP._lock_comp.try_lock()) {QCoreApplication::processEvents(QEventLoop::AllEvents, 10); std::this_thread::sleep_for(std::chrono::milliseconds(100));}
    MLP._lock_comp.unlock();

}


double pgScanGUI::vsConv(val_selector* vs){
    switch(vs->index){
    case 0: return vs->val;
    case 1: return vs->val*1000;
    case 2: return vs->val*vsConv(led_wl);
    case 3: return vs->val*vsConv(coh_len);
    }
}

void pgScanGUI::_correctTilt(scanRes* res, bool force_disable_tilt_correction){
    int titlCorIndex;
    if(force_disable_tilt_correction) titlCorIndex=0;
    else titlCorIndex=tiltCorrection->index;

    if(titlCorIndex==0){
        res->tiltCor[0]=0;
        res->tiltCor[1]=0;
        return;
    }

    int nRows=res->depth.rows;
    int nCols=res->depth.cols;
    cv::Mat mask4hist;
    if(titlCorIndex==1 || getTiltCalibOnNextScan){
        cv::Mat blured;
        double sigma=tiltCorBlur->val;
        int ksize=sigma*5;
        if(!(ksize%2)) ksize++;
        //cv::GaussianBlur(res->depth, blured, cv::Size(ksize, ksize), sigma, sigma);   //using just gaussian blur also blurs out the inf edges masked over, which sometimes causes tilt to fail
        cv::bilateralFilter(res->depth, blured, ksize, sigma, sigma);                   //this does not blur over obvious sharp edges (but is a bit slower)
        double derDbound=tiltCorThrs->val;
        cv::Mat firstD, secondD, mask, maskT;
        cv::Sobel(blured, firstD, CV_32F,1,0); //X
        cv::Sobel(blured, secondD, CV_32F,2,0); //X
        cv::compare(secondD,  derDbound, mask , cv::CMP_LT);
        cv::compare(secondD, -derDbound, maskT, cv::CMP_LE);
        mask.setTo(cv::Scalar::all(0), maskT);
        mask.setTo(cv::Scalar::all(0), res->mask);                                      if(findBaseline->val) mask.copyTo(mask4hist);
        res->tiltCor[0]=-cv::mean(firstD,mask)[0]/8;
        cv::Sobel(blured, firstD, CV_32F,0,1); //Y
        cv::Sobel(blured, secondD, CV_32F,0,2); //Y
        cv::compare(secondD,  derDbound, mask , cv::CMP_LT);
        cv::compare(secondD, -derDbound, maskT, cv::CMP_LE);
        mask.setTo(cv::Scalar::all(0), maskT);
        mask.setTo(cv::Scalar::all(0), res->mask);                                      if(findBaseline->val) cv::bitwise_and(mask4hist,mask,mask4hist);
        res->tiltCor[1]=-cv::mean(firstD,mask)[0]/8;
        std::cout<<"CPhases X,Y: "<<res->tiltCor[0]<< " "<<res->tiltCor[1]<<"\n";
        if(getTiltCalibOnNextScan){
            tiltCor[0]=res->tiltCor[0];
            tiltCor[1]=res->tiltCor[1];
            getTiltCalibOnNextScan=false;
        }
    }else{
        res->tiltCor[0]=tiltCor[0];
        res->tiltCor[1]=tiltCor[1];
    }

    cv::Mat slopeX1(1, nCols, CV_32F);
    cv::Mat slopeX(nRows, nCols, CV_32F);
    for(int i=0;i!=nCols;i++) slopeX1.at<float>(i)=i*res->tiltCor[0];
    cv::repeat(slopeX1, nRows, 1, slopeX);
    cv::add(slopeX,res->depth,res->depth);

    cv::Mat slopeY1(nRows, 1, CV_32F);
    cv::Mat slopeY(nRows, nCols, CV_32F);
    for(int i=0;i!=nRows;i++) slopeY1.at<float>(i)=i*res->tiltCor[1];
    cv::repeat(slopeY1, 1, nCols, slopeY);
    cv::add(slopeY,res->depth,res->depth);

    if(titlCorIndex==1 && findBaseline->val){      //we also want to find the baseline level: we only look at points that are flat enough (some of the original flat image must be visible)
        const double histRes=findBaselineHistStep->val;
        double min,max;
        cv::minMaxLoc(res->depth, &min, &max, nullptr, nullptr, mask4hist);  //the ignored mask values will be <min , everything is in nm
        if(max<=min) max=min+1;
        int hlen=max-min;
        hlen/=histRes;
        int histSize[]={hlen}; float hranges[]={(float)min, (float)max}; const float* ranges[]={hranges}; int channels[]={0};
        cv::Mat hist;
        cv::calcHist(&res->depth, 1, channels, mask4hist, hist, 1, histSize, ranges);
        if(!fnameSaveNextMirrorBaselineHist.empty()){
            std::ofstream wfile(util::toString(fnameSaveNextMirrorBaselineHist,"/baselineHist.txt"));
            for(int i=0;i!=hlen;i++) wfile<<hist.at<float>(i)<<"\n";
            fnameSaveNextMirrorBaselineHist.clear();
            wfile.close();
        }
        cv::Point mpt;
        cv::minMaxLoc(hist, nullptr, nullptr, nullptr, &mpt);
        cv::subtract(res->depth,min+mpt.y*histRes,res->depth);
    }
}

void pgScanGUI::_savePixel(FQ* framequeue, unsigned nFrames, unsigned nDFTFrames){
    if(clickDataLock.try_lock()){
        if(savePix){
            unsigned nRows=framequeue->getUserMat(0)->rows;
            unsigned nCols=framequeue->getUserMat(0)->cols;
            if(clickCoordX>=0 && clickCoordX<nCols && clickCoordY>=0 && clickCoordY<nRows){
                std::ofstream wfile(clickFilename);
                unsigned col=clickCoordX;
                unsigned row=clickCoordY;

                for(unsigned k=0;k!=nFrames;k++){
                    if(nFrames==nDFTFrames){
                        std::complex<double> res(0,0);
                        for(unsigned n=0;n!=nDFTFrames;n++){    // its one pixel, no point optimizing
                            res.real(res.real()+(int)framequeue->getUserMat(n)->at<uint8_t>(row,col)*cos(2*M_PI*n*k/nDFTFrames)*pow(sin(M_PI*n/nDFTFrames),2));
                            res.imag(res.imag()-(int)framequeue->getUserMat(n)->at<uint8_t>(row,col)*sin(2*M_PI*n*k/nDFTFrames)*pow(sin(M_PI*n/nDFTFrames),2));
                        }
                        wfile<<k<<" "<<(int)framequeue->getUserMat(k)->at<uint8_t>(row,col)<<" "<<std::abs(res)<<" "<<std::arg(res)<<"\n";
                    }else{
                        wfile<<k<<" "<<(int)framequeue->getUserMat(k)->at<uint8_t>(row,col)<<"\n";
                    }
                }
                wfile.close();
            }
            savePix=false;
        }
        clickDataLock.unlock();
    }
}

void pgScanGUI::_doOneRound(cv::Rect ROI, char cbAvg_override, bool force_disable_tilt_correction, char cbRefl_override){
    typedef cv::Mat tUMat;//cv::UMat    // TODO fix; use conditional on if opencl was detected
    if(ROI.width==0) ROI=sROI;
    if(ROI.width<=0) ROI=cv::Rect(0,0,go.pGCAM->iuScope->camCols,go.pGCAM->iuScope->camRows);

    MLP._lock_proc.lock();                       //wait for other measurements to complete
    if(!go.pGCAM->iuScope->connected || !go.pRPTY->connected) {measurementInProgress=false; return;}
    if(!CORdy) {measurementInProgress=false; return;}
    pgMGUI->chooseObj(true);    // switch to mirau
    scanRes* res=new scanRes;

    unsigned nFrames=totalFrameNum;
    unsigned nDFTFrames=expectedDFTFrameNum;
    FQ* framequeue;
    go.pGCAM->iuScope->set_trigger("Line1");
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(COfps);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    while(framequeue->getFullNumber()) framequeue->freeUserMat();   // remove any remaining non-trig frames
    COmeasure->execute();
    unsigned cframes, lcframes=0;
    unsigned time=0;
    while(1) {
        cframes=framequeue->getFullNumber();
        if(cframes>=nFrames){
            MLP.progress_proc=100;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if(lcframes==cframes){
            if(cframes==0) time+=10;    // for first frame we put 10x timeout
            else time+=100;
        }else time=0;
        lcframes=cframes;
        if(time>=timeout) break;

        MLP.progress_proc=100./nFrames*cframes;
    }

    framequeue->setUserFps(0);
    go.pGCAM->iuScope->set_trigger("none");


    if(pgMGUI==nullptr) res->XYnmppx=0;
    else res->XYnmppx=pgMGUI->getNmPPx(0);

    pgMGUI->getPos(&res->pos[0], &res->pos[1], &res->pos[2]);
    res->pos[0]+=(-go.pGCAM->iuScope->camCols/2.+ROI.x+ROI.width/2.)*res->XYnmppx/1000000;
    res->pos[1]+=(-go.pGCAM->iuScope->camRows/2.+ROI.y+ROI.height/2.)*res->XYnmppx/1000000;

    std::lock_guard<std::mutex>lock(MLP._lock_comp);    // wait till other processing is done
    MLP._lock_proc.unlock();                             // allow new measurement to start
    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();

    if(framequeue->getFullNumber()!=nFrames){
        Q_EMIT signalQMessageBoxWarning("Error", QString::fromStdString(util::toString("ERROR: took ",framequeue->getFullNumber()," frames, expected ",nFrames,".")));
        go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
        if(MLP._lock_proc.try_lock()){MLP._lock_proc.unlock();measurementInProgress=false;}
        return;
    }

    unsigned nRows=(*framequeue->getUserMat(0))(ROI).rows;
    unsigned nCols=(*framequeue->getUserMat(0))(ROI).cols;


    _savePixel(framequeue, nFrames, nDFTFrames);    //writing pixel to file: for selected pixel

    // for longer scans, here we separate the DFT frames from all taken frames
    tUMat peakIndex;
    if(nDFTFrames!=nFrames){
        MLP.progress_comp=0;
        // get I_0
        tUMat I_0=tUMat::zeros(nRows, nCols, CV_32F);
        for(unsigned n=0;n!=nFrames;n++) cv::add(I_0,(*framequeue->getUserMat(n))(ROI),I_0, cv::noArray(), CV_32F);
        cv::divide(I_0,nFrames,I_0);
        MLP.progress_comp=10;
        // prepare mats
        peakIndex=tUMat::zeros(nRows, nCols, CV_16U);
        tUMat peakMax=tUMat::zeros(nRows, nCols, CV_32F);
        tUMat avg=tUMat::zeros(nRows, nCols, CV_32F);
        tUMat tmp(nRows, nCols, CV_32F);
        tUMat mask(nRows, nCols, CV_8U);
        tUMat mask2(nRows, nCols, CV_8U);
        // calc initial avg
        for(unsigned n=0;n!=nDFTFrames;n++){
            cv::subtract((*framequeue->getUserMat(n))(ROI),I_0,tmp, cv::noArray(), CV_32F);
            cv::multiply(tmp,tmp,tmp);
            cv::add(avg,tmp,avg);
        }
        MLP.progress_comp=20;
        // calc rolling avg
        for(unsigned n=nDFTFrames-1;n!=nFrames;n++){
            if(n!=nDFTFrames-1){
                cv::subtract((*framequeue->getUserMat(n))(ROI),I_0,tmp, cv::noArray(), CV_32F);
                cv::multiply(tmp,tmp,tmp);
                cv::add(avg,tmp,avg);
                cv::subtract((*framequeue->getUserMat(n-nDFTFrames))(ROI),I_0,tmp, cv::noArray(), CV_32F);
                cv::multiply(tmp,tmp,tmp);
                cv::subtract(avg,tmp,avg);
            }
            cv::compare(avg, peakMax, mask, cv::CMP_GT);
            avg.copyTo(peakMax, mask);
            peakIndex.setTo(n-nDFTFrames+1,mask);
        }
        MLP.progress_comp=30;
        tUMat data=tUMat::zeros(nFrames, nRows*nCols, CV_8U);
        tUMat data2=tUMat::zeros(nDFTFrames, nRows*nCols, CV_8U);
        MLP.progress_comp=40;
        unsigned index;
        for(unsigned n=0;n!=nFrames;n++) for(unsigned j=0;j!=nRows;j++)
            (*framequeue->getUserMat(n))(ROI).row(j).copyTo(data.row(n).colRange(j*nCols,(j+1)*nCols));
        MLP.progress_comp=50;
        for(unsigned i=0;i!=nCols;i++) for(unsigned j=0;j!=nRows;j++){
            index=peakIndex.at<uint16_t>(j,i);
            data.col(j*nCols+i).rowRange(index,index+nDFTFrames).copyTo(data2.col(j*nCols+i));
        }
        MLP.progress_comp=60;
        for(unsigned n=0;n!=nDFTFrames;n++) for(unsigned j=0;j!=nRows;j++)
            data2.row(n).colRange(j*nCols,(j+1)*nCols).copyTo((*framequeue->getUserMatNonConst(n))(ROI).row(j));
        MLP.progress_comp=70;
        for(unsigned n=nFrames-1;n!=nDFTFrames-1;n--)
            framequeue->freeUserMat(n);
        MLP.progress_comp=80;
    }

    // this block is for determining wavelength (when scan is called from settings)

    if(getWl){
        getWl=false;
        unsigned npix=scanWlNxNpixels->val;
        std::vector<double> results;
        for(unsigned i=0;i<npix;i++) for(unsigned j=0;j<npix;j++){
            unsigned maxIter=vsConv(pphwl)/2;   // max number of moves while finding the max
            unsigned Nperiods=DFTrange->val-1;  // == peakloc

            unsigned X=nCols/2-npix/2+i;
            unsigned Y=nRows/2-npix/2+j;
            unsigned nDFTFrames=Nperiods*vsConv(pphwl);
            double amp[3];
            for(unsigned k=0;k!=3;k++){
                unsigned frames=nDFTFrames+k-1;
                std::complex<double> res(0,0);
                for(unsigned n=0;n!=frames;n++){    // its one pixel, no point optimizing
                    res.real(res.real()+(int)(*framequeue->getUserMat(n))(ROI).at<uint8_t>(Y,X)*cos(2*M_PI*n*Nperiods/frames)*pow(sin(M_PI*n/frames),2));
                    res.imag(res.imag()-(int)(*framequeue->getUserMat(n))(ROI).at<uint8_t>(Y,X)*sin(2*M_PI*n*Nperiods/frames)*pow(sin(M_PI*n/frames),2));
                }
                amp[k]=1./frames*std::abs(res);
            }
            bool found=false;
            bool direction;
            nDFTFrames+=(amp[0]>amp[1])?-1:1;
            while(!found){
                if(amp[0]>amp[1] || amp[2]>amp[1]){
                    direction=(amp[0]>amp[1]);
                    nDFTFrames+=direction?-1:1;
                    maxIter--;
                    amp[1]=amp[direction?0:2];
                    if(!maxIter) goto next;
                    std::complex<double> res(0,0);
                    for(unsigned n=0;n!=nDFTFrames;n++){
                        res.real(res.real()+(int)(*framequeue->getUserMat(n))(ROI).at<uint8_t>(Y,X)*cos(2*M_PI*n*Nperiods/nDFTFrames)*pow(sin(M_PI*n/nDFTFrames),2));
                        res.imag(res.imag()-(int)(*framequeue->getUserMat(n))(ROI).at<uint8_t>(Y,X)*sin(2*M_PI*n*Nperiods/nDFTFrames)*pow(sin(M_PI*n/nDFTFrames),2));
                    }
                    amp[direction?0:2]=1./nDFTFrames*std::abs(res);
                    if(amp[direction?0:2]<=amp[1]){
                        nDFTFrames-=direction?-1:1;
                        found=true;
                    }
                }else found=true;
            }
            results.push_back(2.*nDFTFrames*displacementOneFrame*1000000/Nperiods);
            MLP.progress_comp=100*(j+i*npix)/(npix*npix);
            next:;
        }
        MLP.progress_comp=100;
        if(results.empty()) Q_EMIT signalQMessageBoxWarning("Error", QString::fromStdString(util::toString("ERROR: failed to correct wavelength.")));
        else{
            double mean=0;
            for(auto& res: results) mean+=res;
            mean/=results.size();
            led_wl->setValue(mean,0);
        }
        go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
        if(MLP._lock_proc.try_lock()){MLP._lock_proc.unlock();measurementInProgress=false;}
        return;
    }


    // real and imaginary accumulation matrices
    tUMat mReal=tUMat::zeros(nRows, nCols, CV_32F);
    tUMat mImag=tUMat::zeros(nRows, nCols, CV_32F);
    // helper matrix:
    tUMat mTmp(nRows, nCols, CV_32F);
    // matrix containing amplitude
    tUMat mAmp(nRows, nCols, CV_32F);
    // matrix containing phase
    tUMat mPhs(nRows, nCols, CV_32F);
    // matrix with mask (1 means bad pixel - invalid mPhs)
    tUMat mask(nRows, nCols, CV_8U);
    tUMat maskNot(nRows, nCols, CV_8U);

    for(unsigned n=0;n!=nDFTFrames;n++){
        cv::multiply((*framequeue->getUserMat(n))(ROI),  cos(2*M_PI*n*peakLoc/nDFTFrames)*pow(sin(M_PI*n/nDFTFrames),2), mTmp, 1, CV_32F);
        cv::add(mReal,mTmp,mReal);
        cv::multiply((*framequeue->getUserMat(n))(ROI), -sin(2*M_PI*n*peakLoc/nDFTFrames)*pow(sin(M_PI*n/nDFTFrames),2), mTmp, 1, CV_32F);
        cv::add(mImag,mTmp,mImag);
        MLP.progress_comp=90*(n+1)/nDFTFrames;
    }
    cv::phase(mReal, mImag, mPhs);  // atan2(y,x)
    if(nDFTFrames!=nFrames){
        cv::compare(mPhs, M_PI, mask, cv::CMP_GE);
        cv::subtract(mPhs,2*M_PI,mPhs,mask);

        cv::multiply(peakIndex, 2*M_PI*peakLoc/nDFTFrames, mTmp, 1, CV_32F);
        cv::subtract(mPhs,mTmp,mPhs);

        if(unwrap->val) do{ // otherwise unwrap fails due to more than 32*pi*pi difference
            cv::compare(mPhs, 0, mask, cv::CMP_LT);
            cv::add(mPhs, 2*M_PI, mPhs,mask);
        }while(cv::countNonZero(mask));
    }
    cv::magnitude(mReal, mImag, mAmp);
    cv::multiply(mAmp, 1./nDFTFrames, mAmp);
    cv::compare(mAmp, (double)dis_thresh->val, mask, cv::CMP_LT);

    if((cbGetRefl->val && cbRefl_override==0) || cbRefl_override==1){
        double min,max;
        cv::minMaxIdx(mAmp,&min,&max);
        mAmp.convertTo(res->refl,CV_8U,((1<<8)-1)/(max-min),-min*((1<<8)-1)/(max-min));        // is amplitude ~ reflectivity of the less reflective mirror of the interferometer
    }

    int dilation_size=exclDill->val;
    if(dilation_size>0){
        cv::Mat element=cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2*dilation_size+1,2*dilation_size+1), cv::Point(dilation_size,dilation_size));
        cv::dilate(mask, mask, element);
    }
    bitwise_not(mask, maskNot);
    mask.copyTo(res->mask);
    maskNot.copyTo(res->maskN);
    mPhs.copyTo(res->depth);

    if(getExpMinMax){
        int minn=256,maxx=-1;
        for(int i=0;i!=framequeue->getFullNumber();i++){
            double min,max;
            cv::minMaxLoc((*framequeue->getUserMat(i))(ROI), &min, &max, 0, 0, res->maskN);
            if(min<minn)minn=min;
            if(max>maxx)maxx=max;
        }
        Q_EMIT doneExpMinmax(minn, maxx);
    }
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

    if(unwrap->val){
        mPhs.copyTo(res->depth);
        cv::phase_unwrapping::HistogramPhaseUnwrapping::Params params;
        params.width=nCols;
        params.height=nRows;
        cv::Ptr<cv::phase_unwrapping::HistogramPhaseUnwrapping> phaseUnwrapping = cv::phase_unwrapping::HistogramPhaseUnwrapping::create(params);
        phaseUnwrapping->unwrapPhaseMap(res->depth, res->depth,res->maskN);    //unfortunately this does not work for UMat (segfaults for ocv 4.1.2), thats why we shuffle the mat back and forth
        res->depth.copyTo(mPhs);
    }
    mPhs.convertTo(mPhs,-1,vsConv(led_wl)/4/M_PI);

    MLP.progress_comp=95;

    mPhs.setTo(std::numeric_limits<float>::max() , mask);
    mPhs.copyTo(res->depth);
    if(useCorr->try_lock()){
        if(cor==nullptr)std::cerr<<"Correction was nullptr!\n";
        else if(cor->depth.empty())std::cerr<<"Correction depthmap was empty!\n";
        else if(res->depth.cols==cor->depth(ROI).cols && res->depth.rows==cor->depth(ROI).rows) cv::subtract(res->depth,cor->depth(ROI),res->depth,res->maskN);
        else std::cerr<<"Cannot do correction: the measured and correction depthmaps are of different dimensions!\n";
        useCorr->unlock();
    }

    res->avgNum=1;

    if(bSaveAvgMess){  // for autosaving raw data - for debug and bookeeping purposes - for saving individual measurements that are being averaged.
        saveScan(res,util::toString(stringSaveAvgMess,'/',saveIter),true);
        saveIter++;
    }

    if((cbAvg->val && cbAvg_override==0) || cbAvg_override==1){         // averaging multiple measurements
        varShareClient<pgScanGUI::scanRes>* temp=result.getClient();
        const scanRes* oldScanRes=temp->get();
        if(oldScanRes==nullptr);
        else if(oldScanRes->pos[0]!=res->pos[0] || oldScanRes->pos[1]!=res->pos[1] || oldScanRes->XYnmppx!=res->XYnmppx || oldScanRes->depth.cols!=nCols || oldScanRes->depth.rows!=nRows || skipAvgSettingsChanged) skipAvgSettingsChanged=false;
            //we moved or scan settings changed, no point to make average
        else{
            // first we decorrect tilt on old mes
            cv::Mat corDepth;
            oldScanRes->depth.copyTo(corDepth);
            if(oldScanRes->tiltCor[0]!=0){
                cv::Mat slopeX1(1, nCols, CV_32F);
                cv::Mat slopeX(nRows, nCols, CV_32F);
                for(int i=0;i!=nCols;i++) slopeX1.at<float>(i)=-i*(oldScanRes->tiltCor[0]);
                cv::repeat(slopeX1, nRows, 1, slopeX);
                cv::add(slopeX,corDepth,corDepth);
            }
            if(oldScanRes->tiltCor[1]!=0){
                cv::Mat slopeY1(nRows, 1, CV_32F);
                cv::Mat slopeY(nRows, nCols, CV_32F);
                for(int i=0;i!=nRows;i++) slopeY1.at<float>(i)=-i*(oldScanRes->tiltCor[1]);
                cv::repeat(slopeY1, 1, nCols, slopeY);
                cv::add(slopeY,corDepth,corDepth);
            }

            //for iterative calculations we use Donald Knuth's "The Art of Computer Programming, Volume 2: Seminumerical Algorithms", section 4.2.2
            //  M(1) = x(1), M(k) = M(k-1) + (x(k) - M(k-1)) / k
            //  S(1) = 0, S(k) = S(k-1) + (x(k) - M(k-1)) * (x(k) - M(k))
            //  for 2 <= k <= n, then
            //  sigma = sqrt(S(n) / (n - 1))

            //we calc the mean
            if(cv::countNonZero(res->mask)-cv::countNonZero(oldScanRes->mask) > res->mask.total()*avgDiscardCriteria->val/100){  //the new measurement has too many bad pixels compared to old res - we discard
                std::cerr<<"Too many bad pixels, skipping.\n";
                std::cerr<<"done nr "<<NA<<"\n"; NA++;
                MLP.progress_comp=100;
                if(MLP._lock_proc.try_lock()){MLP._lock_proc.unlock();measurementInProgress=false;}
                delete res;
                return;
            }

            res->mask.setTo(255,oldScanRes->mask);
            bitwise_not(res->mask, res->maskN);

            if(avgEnableAntiDrift->val){    // correct for drift
                tUMat temp;
                tUMat mask;
                cv::dilate(res->mask,mask,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(1,1)));
                double shift=avgAntiDriftStep->val;
                double shiftX=0, shiftY=0;
                double nshiftX=0, nshiftY=0;
                double lacor,acor;

                //first get no shift anticorrelation
                cv::absdiff(res->depth,corDepth,temp);
                temp.setTo(0,mask);
                lacor=cv::sum(temp)[0];

                const double mshift[5][4][2]={
                    {/*right*/  {shift,0},  /*up*/       {0,shift},  /*left*/    {-shift,0},     /*down*/    {0,-shift}},
                    {/*right*/  {shift,0},  /*up*/       {0,shift},  /*down*/    {0,-shift},     /*none*/    {0,0     }},
                    {/*up*/     {0,shift},  /*left*/    {-shift,0},  /*right*/   {shift,0},      /*none*/    {0,0     }},
                    {/*left*/  {-shift,0},  /*down*/    {0,-shift},  /*up*/      {0,shift},      /*none*/    {0,0     }},
                    {/*down*/  {0,-shift},  /*right*/    {shift,0},  /*left*/    {-shift,0},     /*none*/    {0,0     }}
                };
                const int lastmoves[5][4]={
                    {/*right*/  1,  /*up*/       2,  /*left*/    3,     /*down*/    4},
                    {/*right*/  1,  /*up*/       2,  /*down*/    4,     /*none*/    0},
                    {/*up*/     2,  /*left*/     4,  /*right*/   1,     /*none*/    0},
                    {/*left*/   3,  /*down*/     4,  /*up*/      2,     /*none*/    0},
                    {/*down*/   4,  /*right*/    1,  /*left*/    3,     /*none*/    0}
                };
                uchar lastmove=0;       // 0=none, 1=right, 2=up, 3=left, 4=down
                bool cont;
                while(1){
                    cont=false;
                    for(int i=0;i!=4;i++){
                        if(i==3 && lastmove!=0) break;
                        nshiftX=shiftX+mshift[lastmove][i][0];
                        nshiftY=shiftY+mshift[lastmove][i][1];
                        if(abs(nshiftX)>=1 || abs(nshiftY)>=1) break;
                        acor=avgDriftCorr(&corDepth, &res->depth, &mask, &temp, nshiftX, nshiftY);
                        //std::cerr<<"did one driftcortest\n";
                        //std::cerr<<std::setprecision(10)<<"last acor="<<lacor<<", new acor="<<acor<<"\n";
                        if(acor<lacor){
                            shiftX=nshiftX;
                            shiftY=nshiftY;
                            lacor=acor;
                            lastmove=lastmoves[lastmove][i];
                            //std::cerr<<" was better\n";
                            cont=true;
                        }
                    }
                    if(cont) continue;
                    break;
                }
                //std::cerr<<"shiftX="<<shiftX<<", shiftY="<<shiftY<<"\n";
                if(shiftX!=0 || shiftY!=0){
                    cv::Mat trans=(cv::Mat_<float>(2,3) << 1, 0, shiftX, 0, 1, shiftY);
                    cv::warpAffine(res->depth, res->depth, trans, res->depth.size(), cv::INTER_LINEAR, cv::BORDER_REFLECT);  // TODO INTER_LINEAR should be INTER_LINEAR_EXACT but it crashes this version of openvc
                }
            }

            res->depth.setTo(0,res->mask);
            cv::Mat tempp;
            corDepth.copyTo(tempp);
            tempp.setTo(0,res->mask);
            double avgOld=(double)cv::sum(tempp)[0]/cv::countNonZero(res->maskN);
            double avgNew=(double)cv::sum(res->depth)[0]/cv::countNonZero(res->maskN);
            cv::add(res->depth,avgOld-avgNew,res->depth);

            cv::Mat newDepth;
            res->depth.copyTo(newDepth);    //we need this later for SD

            res->avgNum+=oldScanRes->avgNum;
            cv::subtract(res->depth,corDepth,res->depth);
            cv::addWeighted(res->depth,1./res->avgNum,corDepth,1,0,res->depth);
            res->depth.setTo(std::numeric_limits<float>::max(),res->mask);


            //now calc the (!)SD
            cv::subtract(newDepth,corDepth,res->depthSS);      // (Xk - Mk-1)
            cv::subtract(newDepth,res->depth,newDepth);                 // (Xk - Mk)
            cv::multiply(res->depthSS,newDepth,res->depthSS);           // (Xk - Mk-1)(Xk-Mk)
            if(oldScanRes->avgNum>2)
                cv::add(res->depthSS,oldScanRes->depthSS,res->depthSS); // Sk-1 + (Xk - Mk-1)(Xk-Mk)
            res->depthSS.setTo(std::numeric_limits<float>::max(),res->mask);

            std::cerr<<"AVG: done num. "<<res->avgNum<<"\n";
        }
        delete temp;
    }

    _correctTilt(res, force_disable_tilt_correction);     //autocorrect tilt

    cv::minMaxLoc(res->depth, &res->min, &res->max, nullptr, nullptr, res->maskN);  //the ignored mask values will be <min , everything is in nm

    result.put(res);

    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"Calculation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
    std::cerr<<"done nr "<<NA<<"\n"; NA++;
    std::cerr<<"Full: "<<go.pGCAM->iuScope->FQsPCcam.getFullNumber()<<"\n";

    MLP.progress_comp=100;
    if(MLP._lock_proc.try_lock()){MLP._lock_proc.unlock();measurementInProgress=false;}
}

double pgScanGUI::avgDriftCorr(const cv::Mat* mavg, const cv::Mat* mnew, const cv::Mat* mask, cv::Mat* temp, double shiftX, double shiftY){
    cv::Mat trans=(cv::Mat_<float>(2,3) << 1, 0, shiftX, 0, 1, shiftY);
    std::cerr<<trans;
    cv::warpAffine(*mnew, *temp, trans, mnew->size(), cv::INTER_LINEAR, cv::BORDER_REFLECT);  // TODO INTER_LINEAR should be INTER_LINEAR_EXACT but it crashes this version of openvc
    cv::absdiff(*temp,*mavg,*temp);
    temp->setTo(0,*mask);
    return cv::sum(*temp)[0];
}

// SOME STATIC FUNCTIONS

QWidget* pgScanGUI::parent{nullptr};
cv::Rect pgScanGUI::lastROI{0,0,0,0};
void pgScanGUI::saveScan(const scanRes* scan, std::string fileName, bool useLastROI, bool saveSD, bool saveRF){
    if(useLastROI && lastROI!=cv::Rect(0,0,0,0)) pgScanGUI::saveScan(scan, lastROI, fileName, saveSD, saveRF);
    else pgScanGUI::saveScan(scan, {0,0,scan->depth.cols,scan->depth.rows}, fileName, saveSD, saveRF);
}
void pgScanGUI::saveScan(const scanRes* scan, const cv::Rect &roi, std::string fileName, bool saveSD, bool saveRF){
    lastROI=roi;
    if(scan==nullptr) return;
    if(fileName=="") fileName=QFileDialog::getSaveFileName(parent,"Select file for saving Depth Map (raw, float).", "","Images (*.pfm)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".pfm")==std::string::npos) fileName+=".pfm";

    cv::imwrite(fileName, cv::Mat(scan->depth, roi));
    double min, max;
    cv::minMaxLoc(cv::Mat(scan->depth, roi), &min, &max, nullptr, nullptr, cv::Mat(scan->maskN, roi));
    std::ofstream wfile(fileName, std::ofstream::app);
    wfile.write(reinterpret_cast<const char*>(&min),sizeof(min));
    wfile.write(reinterpret_cast<const char*>(&max),sizeof(max));
    wfile.write(reinterpret_cast<const char*>(scan->tiltCor),sizeof(scan->tiltCor));
    if(roi.x==0 && roi.y==0){   // NOTE: the save posX and posY point in the opposite direction of the image matrix iterators!
        wfile.write(reinterpret_cast<const char*>(scan->pos),sizeof(scan->pos));
    }else{  //gotta correct image coordinates for cropped images...
        double pos[3];
        for(int i=0;i!=3;i++)  pos[i]=scan->pos[i];
        pos[0]-=roi.x*scan->XYnmppx/1e6;
        pos[1]-=roi.y*scan->XYnmppx/1e6;
        wfile.write(reinterpret_cast<const char*>(pos),sizeof(pos));
    }
    wfile.write(reinterpret_cast<const char*>(&(scan->XYnmppx)),sizeof(scan->XYnmppx));
    wfile.write(reinterpret_cast<const char*>(&(scan->avgNum)),sizeof(scan->avgNum));
    wfile.close();

    if(!scan->depthSS.empty() && scan->avgNum>1 && saveSD){
        std::string nfileName=fileName;
        nfileName.insert(fileName.length()-4,"-SD");
        cv::imwrite(nfileName, cv::Mat(scan->depthSS, roi));
    }
    if(!scan->refl.empty() && saveRF){
        std::string nfileName=fileName;
        nfileName.replace(nfileName.end()-4,nfileName.end(),"-RF.png");
        cv::imwrite(nfileName, cv::Mat(scan->refl, roi));
    }
}
void pgScanGUI::saveScanTxt(const scanRes* scan, std::string fileName){
    pgScanGUI::saveScanTxt(scan, {0,0,scan->depth.cols,scan->depth.rows}, fileName);
}
void pgScanGUI::saveScanTxt(const scanRes* scan, const cv::Rect &roi, std::string fileName){
    if(scan==nullptr) return;
    if(fileName=="") fileName=QFileDialog::getSaveFileName(parent,"Select file for saving map in text.", "","Text (*.txt)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".txt")==std::string::npos) fileName+=".txt";

    std::ofstream wfile(fileName);
    cv::Mat display(scan->depth, roi);
    double min, max;
    cv::minMaxLoc(cv::Mat(scan->depth, roi), &min, &max, nullptr, nullptr, cv::Mat(scan->maskN, roi));
    wfile<<util::toString("# XY is in pixels, see below for conversion constant. Depth is in nm!\n");
    wfile<<util::toString("# For how to plot with gnuplot see http://www.gnuplotting.org/interpolation-of-heat-maps/\n");
    wfile<<util::toString("# Min value: ",min,"\n");
    wfile<<util::toString("# Max value: ",max,"\n");
    wfile<<util::toString("# Applied X Tilt Correction: ",scan->tiltCor[0],"\n");
    wfile<<util::toString("# Applied Y Tilt Correction: ",scan->tiltCor[1],"\n");
    wfile<<util::toString("# X during measurement: ",scan->pos[0]," mm\n");
    wfile<<util::toString("# Y during measurement: ",scan->pos[1]," mm\n");
    wfile<<util::toString("# Z during measurement: ",scan->pos[2]," mm\n");
    wfile<<util::toString("# XY nm per px: ",scan->XYnmppx,"\n");
    wfile<<util::toString("# Number of averaged frames: ",scan->avgNum,"\n");

    for(int j=display.rows-1;j>=0;j--){
        for(int i=0;i!=display.cols;i++){
            if(i!=0) wfile<<" ";
            if(display.at<float>(j,i)==std::numeric_limits<float>::max()) wfile<<"nan";
            else wfile<<display.at<float>(j,i);
        }
        wfile<<"\n";
    }
    wfile.close();

    if(!scan->depthSS.empty()){
        fileName.insert(fileName.length()-4,"-SD");
        std::ofstream wfile(fileName);

        cv::Mat display2;
        cv::divide(cv::Mat(scan->depthSS,roi),scan->avgNum-1,display2);
        cv::sqrt(display2,display2);

        wfile<<util::toString("# This file contains the SD\n");

        for(int j=display2.rows-1;j>=0;j--){
            for(int i=0;i!=display2.cols;i++){
                if(i!=0) wfile<<" ";
                if(display2.at<float>(j,i)==std::numeric_limits<float>::max()) wfile<<"nan";
                else wfile<<display2.at<float>(j,i);
            }
            wfile<<"\n";
        }
        wfile.close();
    }
}
bool pgScanGUI::loadScan(scanRes* scan, std::string fileName){
    if(fileName=="") fileName=QFileDialog::getOpenFileName(parent,"Select file to load Depth Map (raw, float).", "","Images (*.pfm)").toStdString();
    if(fileName.empty())return false;
    bool newSR{false};
    if(scan==nullptr) {newSR=true; scan=new scanRes;}
    scan->depth=cv::imread(fileName, cv::IMREAD_UNCHANGED);
    if(scan->depth.empty()){if(newSR) {delete scan; scan=nullptr;} return false;}
    cv::compare(scan->depth, std::numeric_limits<float>::max(), scan->mask, cv::CMP_EQ);
    cv::bitwise_not(scan->mask, scan->maskN);
    std::ifstream rfile(fileName);
    for(int i=0;i!=3;i++) rfile.ignore(256,'\n');                   //skip the first three lines of image header
    rfile.ignore(scan->depth.total()*scan->depth.elemSize(),EOF);   //skip image
    rfile.read(reinterpret_cast<char*>(&(scan->min)),sizeof(scan->min));            if(!rfile) scan->min=0;     //if missing, backward compatibility
    rfile.read(reinterpret_cast<char*>(&(scan->max)),sizeof(scan->max));            if(!rfile) scan->max=1;
    rfile.read(reinterpret_cast<char*>(scan->tiltCor),sizeof(scan->tiltCor));       if(!rfile) for(int i=0;i!=2;i++) scan->tiltCor[i]=0;
    rfile.read(reinterpret_cast<char*>(scan->pos),sizeof(scan->pos));               if(!rfile) for(int i=0;i!=3;i++) scan->pos[i]=0;
    rfile.read(reinterpret_cast<char*>(&(scan->XYnmppx)),sizeof(scan->XYnmppx));    if(!rfile) scan->XYnmppx=1;
    rfile.read(reinterpret_cast<char*>(&(scan->avgNum)),sizeof(scan->avgNum));      if(!rfile) scan->avgNum=1;
    rfile.close();

    if(scan->avgNum>1){
        std::string nfileName=fileName;
        nfileName.insert(fileName.length()-4,"-SD");
        scan->depthSS=cv::imread(nfileName, cv::IMREAD_UNCHANGED);
    }
    {
        std::string nfileName=fileName;
        nfileName.replace(nfileName.end()-4,nfileName.end(),"-RF.png");
        scan->refl=cv::imread(nfileName, cv::IMREAD_UNCHANGED);
    }
    return true;
}
pgScanGUI::scanRes pgScanGUI::difScans(scanRes* scan0, scanRes* scan1){
    scanRes ret;
    if(scan0==nullptr || scan1==nullptr) {std::cerr<<"difScans got nullptr input\n"; return ret;}

    int nCols=std::max(scan0->depth.cols,scan1->depth.cols);
    int nRows=std::max(scan0->depth.rows,scan1->depth.rows);
    cv::Mat _mask0(nRows, nCols, CV_8U, cv::Scalar(255));
    cv::Mat _mask1(nRows, nCols, CV_8U, cv::Scalar(255));
    cv::Mat _depth0(nRows, nCols, CV_32F, cv::Scalar(0));
    cv::Mat _depth1(nRows, nCols, CV_32F, cv::Scalar(0));

    scan0->depth.copyTo(_depth0(cv::Rect(0,0,scan0->depth.cols,scan0->depth.rows)));
    scan0->mask.copyTo(_mask0(cv::Rect(0,0,scan0->mask.cols,scan0->mask.rows)));
    scan1->depth.copyTo(_depth1(cv::Rect(0,0,scan1->depth.cols,scan1->depth.rows)));
    scan1->mask.copyTo(_mask1(cv::Rect(0,0,scan1->mask.cols,scan1->mask.rows)));

    float xShft, yShft;
    float xShiftFrac=modf(xDifShift->val, &xShft);
    float yShiftFrac=modf(yDifShift->val, &yShft);

    cv::Mat depth0, depth1, mask0, mask1;
    if(xShft>= nCols) xShft=nCols-1;
    if(xShft<=-nCols) xShft=1-nCols;
    if(yShft>= nRows) yShft=nRows-1;
    if(yShft<=-nRows) yShft=1-nRows;
    depth0=_depth0(cv::Rect(xShft>0?0:abs(xShft), yShft>0?0:abs(yShft), nCols-abs(xShft), nRows-abs(yShft)));
    mask0 = _mask0(cv::Rect(xShft>0?0:abs(xShft), yShft>0?0:abs(yShft), nCols-abs(xShft), nRows-abs(yShft)));
    depth1=_depth1(cv::Rect(xShft>0?abs(xShft):0, yShft>0?abs(yShft):0, nCols-abs(xShft), nRows-abs(yShft)));
    mask1 = _mask1(cv::Rect(xShft>0?abs(xShft):0, yShft>0?abs(yShft):0, nCols-abs(xShft), nRows-abs(yShft)));

    if(xShiftFrac!=0 || yShiftFrac!=0){
        cv::Mat trans=(cv::Mat_<double>(2,3) << 1, 0, xShiftFrac, 0, 1, yShiftFrac);
        cv::warpAffine(depth0, depth0, trans, depth0.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        cv::dilate(mask0,mask0,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(1,1)));
        cv::Rect cut(cv::Rect(1, 1, depth0.cols-2, depth0.rows-2));
        depth0=depth0(cut); mask0 =mask0 (cut);
        depth1=depth1(cut); mask1 =mask1 (cut);
    }

    cv::subtract(depth1, depth0, ret.depth);
    nCols=depth0.cols;
    nRows=depth0.rows;
    if(scan1->tiltCor[0]-scan0->tiltCor[0]!=0){
        cv::Mat slopeX1(1, nCols, CV_32F);
        cv::Mat slopeX(nRows, nCols, CV_32F);
        for(int i=0;i!=nCols;i++) slopeX1.at<float>(i)=-i*(scan1->tiltCor[0]-scan0->tiltCor[0]);
        cv::repeat(slopeX1, nRows, 1, slopeX);
        cv::add(slopeX,ret.depth,ret.depth);
    }
    if(scan1->tiltCor[1]-scan0->tiltCor[1]!=0){
        cv::Mat slopeY1(nRows, 1, CV_32F);
        cv::Mat slopeY(nRows, nCols, CV_32F);
        for(int i=0;i!=nRows;i++) slopeY1.at<float>(i)=-i*(scan1->tiltCor[1]-scan0->tiltCor[1]);
        cv::repeat(slopeY1, 1, nCols, slopeY);
        cv::add(slopeY,ret.depth,ret.depth);
    }
    mask0.copyTo(ret.mask);
    ret.mask.setTo(255, mask1);
    bitwise_not(ret.mask, ret.maskN);
    ret.depth.setTo(std::numeric_limits<float>::max(), ret.mask);
    cv::minMaxLoc(ret.depth, &ret.min, &ret.max, nullptr, nullptr, ret.maskN);
    for(int i=0;i!=3;i++) ret.pos[i]=scan0->pos[i];
    ret.XYnmppx=scan0->XYnmppx;
    ret.tiltCor[0]=0; ret.tiltCor[1]=0;
    ret.avgNum=scan0->avgNum;
    return ret;
}

pgScanGUI::scanRes pgScanGUI::avgScans(std::vector<scanRes>* scans, double excludeSDfactor, int maxNofCorrections){
    std::cerr<<"Averaging scans...\n";
    scanRes ret;
    int corDone{0};
    if(scans==nullptr) {std::cerr<<"avgScans got nullptr vector input\n"; return ret;}
    for(auto& scan0:*scans) for(auto& scan1:*scans)
        if(scan0.depth.cols!=scan1.depth.cols || scan0.depth.rows!=scan1.depth.rows) {std::cerr<<"avgScans got matrices with nonequal dimensions\n"; return ret;}
    cv::Mat counts;

    ret.mask =cv::Mat(scans->at(0).depth.rows,scans->at(0).depth.cols,CV_8U,cv::Scalar(0));
    ret.maskN=cv::Mat(scans->at(0).depth.rows,scans->at(0).depth.cols,CV_8U,cv::Scalar(255));

    std::vector<cv::Mat> localMasks;    //to return the same scans
    for(int i=0;i!=scans->size();i++) localMasks.emplace_back(scans->at(i).mask.clone());

redo: ret.depth=cv::Mat(scans->at(0).depth.rows,scans->at(0).depth.cols,CV_32F,cv::Scalar(0));
    ret.depthSS=cv::Mat(scans->at(0).depth.rows,scans->at(0).depth.cols,CV_32F,cv::Scalar(0));      //we'll put SD in here
    counts=cv::Mat(scans->at(0).depth.rows,scans->at(0).depth.cols,CV_32F,cv::Scalar(0));
    for(auto& scan:*scans){         //we get the first avg
        cv::add(ret.depth, scan.depth, ret.depth, scan.maskN);
        cv::add(counts, cv::Scalar(1), counts, scan.maskN);
    }
    cv::divide(ret.depth,counts,ret.depth);

    cv::Mat makeZero;
    cv::compare(counts, cv::Scalar(0), makeZero, cv::CMP_EQ);   //we make sure that pixels that have no elements are set to 0, to prevent crashes
    ret.depth.setTo(cv::Scalar(0), makeZero);
    ret.depthSS.setTo(cv::Scalar(0), makeZero);

    counts=cv::Mat(scans->at(0).depth.rows,scans->at(0).depth.cols,CV_32F,cv::Scalar(0));
    for(auto& scan:*scans){         //first make them have the same background level
        cv::Mat temp;
        cv::subtract(scan.depth, ret.depth, temp, scan.maskN);
        cv::Scalar mean=cv::mean(temp, scan.maskN);
        cv::subtract(scan.depth, mean, scan.depth, scan.maskN);
    }
    for(auto& scan:*scans){         //calc SD
        cv::Mat temp;
        cv::subtract(scan.depth, ret.depth, temp, scan.maskN);
        cv::pow(temp,2,temp);
        cv::add(ret.depthSS, temp, ret.depthSS, scan.maskN);
        cv::add(counts, cv::Scalar(1), counts, scan.maskN);
    }
    cv::divide(ret.depthSS,counts,ret.depthSS);
    cv::sqrt(ret.depthSS,ret.depthSS);

    if(excludeSDfactor>0 && corDone<maxNofCorrections){     //now we should do some corrections: remove the measurements that are too far from the avg
        int rmd{0};
        for(auto& scan:*scans){         //now we disable pixels that are excludeSDfactor times farther from mean than SD
            cv::Mat temp, mask;
            cv::absdiff(scan.depth, ret.depth, temp);
            cv::multiply(temp,1/excludeSDfactor,temp);
            cv::compare(temp, ret.depthSS, mask, cv::CMP_GT);
            mask.setTo(cv::Scalar(0),scan.mask);
            scan.maskN.setTo(cv::Scalar(0),mask);
            cv::bitwise_not(scan.maskN,scan.mask);
            rmd+=cv::countNonZero(mask);
        }
        if(rmd!=0){      //we excluded at least one pixel
            std::cerr<<"Found "<<rmd<<" pixels too far from mean, removing and correcting (cor #"<<corDone<<")...\n";
            corDone++;
            goto redo;
        }
    }

    cv::minMaxLoc(ret.depth, &ret.min, &ret.max, nullptr, nullptr);
    for(int i=0;i!=3;i++) ret.pos[i]=scans->at(0).pos[i];
    ret.XYnmppx=scans->at(0).XYnmppx;
    ret.tiltCor[0]=0; ret.tiltCor[1]=0;
    ret.avgNum=scans->size();

    for(int i=0;i!=scans->size();i++){      //since we modify the masks in this function while correcting, we turn them back here
        localMasks[i].copyTo(scans->at(i).mask);
        cv::bitwise_not(scans->at(i).mask,scans->at(i).maskN);
    }
    _correctTilt(&ret);
    cv::minMaxLoc(ret.depth, &ret.min, &ret.max, nullptr, nullptr, ret.maskN);
    return ret;
}

void pgScanGUI::onBSaveAvgMess(){
    if(bSaveAvgMess)
        bSaveAvgMess=false;
    else{
        stringSaveAvgMess=QFileDialog::getExistingDirectory(gui_settings, "Select Folder for Saving Raw Images").toStdString();
        if(!stringSaveAvgMess.empty()) {bSaveAvgMess=true; saveIter=0;}
    }
}

void pgScanGUI::onBSaveNextMirrorBaselineHist(){
    fnameSaveNextMirrorBaselineHist=QFileDialog::getExistingDirectory(gui_settings, "Select folder for saving next mirror baseline histogram.").toStdString();
}
