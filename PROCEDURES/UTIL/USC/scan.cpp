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

pgScanGUI::pgScanGUI(mesLockProg& MLP): MLP(MLP){
    PVTmeasure=go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_PVTmeasure.txt").c_str());
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
}

void pgScanGUI::init_gui_activation(){
    gui_activation=new twid(false);
    bScan=new QPushButton;
    bScan->setText("Scan");
    connect(bScan, SIGNAL(released()), this, SLOT(onBScan()));
    bScanContinuous=new QCheckBox;
    bScanContinuous->setText("Repeat");
    connect(bScanContinuous, SIGNAL(toggled(bool)), this, SLOT(onBScanContinuous(bool)));
    gui_activation->addWidget(bScan);
    gui_activation->addWidget(bScanContinuous);
    cbCorrectTilt=new checkbox_gs(false,"Fix Tilt");
    conf["cbCorrectTilt"]=cbCorrectTilt;
    gui_activation->addWidget(cbCorrectTilt);
    cbAvg=new checkbox_gs(false,"Average");
    conf["cbAvg"]=cbAvg;
    gui_activation->addWidget(cbAvg);
    cbGetRefl=new checkbox_gs(false,"Reflectivity");
    conf["cbGetRefl"]=cbGetRefl;
    gui_activation->addWidget(cbGetRefl);

    xDifShift=new QDoubleSpinBox(); xDifShift->setRange(-1000,1000); xDifShift->setSuffix(" px"); xDifShift->setDecimals(2);
    yDifShift=new QDoubleSpinBox(); yDifShift->setRange(-1000,1000); yDifShift->setSuffix(" px"); xDifShift->setDecimals(2);
    gui_processing=new twid(new QLabel("dif. Shift (x,y): "),xDifShift,yDifShift);
    gui_processing->setToolTip("This applies to all functions that used dif. scans!");
}
void pgScanGUI::onBScanContinuous(bool status){bScan->setCheckable(status);}
void pgScanGUI::onBScan(){
    if(bScan->isCheckable() && !bScan->isChecked());
    else if(MLP._lock_meas.try_lock()){MLP._lock_meas.unlock();doOneRound();}
    if(bScan->isCheckable() && bScan->isChecked()) timerCM->start();
}
void pgScanGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    selectScanSetting=new smp_selector("Select scan setting: ", 0, {"Standard", "Precise", "Set2", "Set3", "Set4"});    //should have Nset strings
    conf["selectScanSetting"]=selectScanSetting;
    slayout->addWidget(selectScanSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new scanSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    connect(selectScanSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    debugDisplayModeSelect=new smp_selector("Display mode select (for debugging purposes): ", 0, {"Unwrapped", "Wrapped"});
    slayout->addWidget(debugDisplayModeSelect);
    avgDiscardCriteria=new val_selector(5.0, "Averaging Mask% Discard Threshold:", 0.1, 100., 1, 1, {"%"});
    conf["avgDiscardCriteria"]=avgDiscardCriteria;
    avgDiscardCriteria->setToolTip("If the difference in the number of excluded element between the current measurement and the avg measurement (which combines all previous exclusions(or)) is larger than this percentage times the total num of pixels, the measurement is discarded.");
    slayout->addWidget(avgDiscardCriteria);
    saveAvgMess=new QPushButton("Autosave raw measurements"); saveAvgMess->setCheckable(true);
    saveAvgMess->setToolTip("Select folder for saving all subsequent individual raw measurements. Last ROI applies (ie you must try saving something raw by selection first), else whole image is saved. Click again to disable further saving.");
    slayout->addWidget(new twid(saveAvgMess));
    saveNextMirrorBaselineHist=new QPushButton("Save next mirror baseline histogram.");
    slayout->addWidget(new twid(saveNextMirrorBaselineHist));
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
    led_wl=new val_selector(470., "LED Wavelength:", 0.001, 2000., 2, 0, {"nm","um"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["led_wl"]=led_wl;
    connect(led_wl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(led_wl);
    coh_len=new val_selector(20., "Coherence Length L:", 1., 2000., 2, 2, {"nm","um",QChar(0x03BB)});
    parent->conf[parent->selectScanSetting->getLabel(num)]["coh_len"]=coh_len;
    connect(coh_len, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(coh_len);
    range=new val_selector(10., "Scan Range:", 0.01, 2000., 5, 3 , {"nm","um",QChar(0x03BB),"L"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["range"]=range;
    connect(range, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(range);
    ppwl=new val_selector(20., "Points Per Wavelength: ", 6, 2000., 2);
    parent->conf[parent->selectScanSetting->getLabel(num)]["ppwl"]=ppwl;
    connect(ppwl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(ppwl);
    max_vel=new val_selector(300., "UScope stage max velocity: ", 1e-9, 300., 2, 0, {"mm/s"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["max_vel"]=max_vel;
    connect(max_vel, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(max_vel);
    max_acc=new val_selector(2500., "UScope stage max acceleration: ", 1e-9, 2500., 2, 0, {"mm/s^2"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["max_acc"]=max_acc;
    connect(max_acc, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(max_acc);
    dis_thresh=new val_selector(0.9, "Peak Discard Threshold: ", 0.1, 1, 2);
    parent->conf[parent->selectScanSetting->getLabel(num)]["dis_thresh"]=dis_thresh;
    slayout->addWidget(dis_thresh);
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
    findBaseline=new checkbox_gs(false,"Correct for mirror baseline.");
    parent->conf[parent->selectScanSetting->getLabel(num)]["tab_camera_findBaseline"]=findBaseline;
    slayout->addWidget(findBaseline);
    findBaselineHistStep=new val_selector(0.1, "Mirror baseline cor. hist. step: ", 0.01, 10, 2, 0, {"nm"});
    parent->conf[parent->selectScanSetting->getLabel(num)]["findBaselineHistStep"]=findBaselineHistStep;
    slayout->addWidget(findBaselineHistStep);
}
void pgScanGUI::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    led_wl=settingWdg[index]->led_wl;
    coh_len=settingWdg[index]->coh_len;
    range=settingWdg[index]->range;
    ppwl=settingWdg[index]->ppwl;
    max_vel=settingWdg[index]->max_vel;
    max_acc=settingWdg[index]->max_acc;
    dis_thresh=settingWdg[index]->dis_thresh;
    calcL=settingWdg[index]->calcL;
    exclDill=settingWdg[index]->exclDill;
    tiltCorBlur=settingWdg[index]->tiltCorBlur;
    tiltCorThrs=settingWdg[index]->tiltCorThrs;
    findBaseline=settingWdg[index]->findBaseline;
    findBaselineHistStep=settingWdg[index]->findBaselineHistStep;
    recalculate();
}

void pgScanGUI::recalculate() {
    if(MLP._lock_meas.try_lock()){
        if(MLP._lock_comp.try_lock()){
            std::string report;
            updatePVTs(report);
            calcL->setText(report.c_str());
            MLP._lock_comp.unlock();
        }
        else timer->start();
        MLP._lock_meas.unlock();
    }
    else timer->start();
}

void pgScanGUI::updatePVTs(std::string &report){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    double minFPS,maxFPS;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &maxFPS);
    double readTime=vsConv(range)*vsConv(ppwl)/vsConv(led_wl)/maxFPS*2;   //s
    double readRangeDis=vsConv(range)/1000000;
    double readVelocity=readRangeDis/readTime;                          //mm/s
    report+=util::toString("Read Time =",readTime," s\nRead Range Distance:",readRangeDis," mm\nRead Velocity:",readVelocity," m/s\n");
    if(readVelocity>vsConv(max_vel)) {
        report+="Error: Read Velocity is higher than set max Velocity!\n";
        return;
    }
    double readAccelTime=readVelocity/vsConv(max_acc);
    double readAccelDis=vsConv(max_acc)/2*readAccelTime*readAccelTime;
    double Offset=readRangeDis/2+readAccelDis;
    PVTsRdy=false;
    PVTmeasure->clear();

    double movTime=2*sqrt(2*Offset/vsConv(max_acc));
    double movMaxVelocity=vsConv(max_acc)*movTime;

    report+=util::toString("Read Acceleration Time :",readAccelTime," s\nRead Acceleration Distance:",readAccelDis," mm\nOffset:",Offset," mm\nMovement Time:",movTime," s\nMovement Max Velocity:",movMaxVelocity," m/s\n");
    report+=util::toString("Total movement Time:",movTime," s\n");
    if(movMaxVelocity > vsConv(max_vel)){report+="Error: Max move Velocity is higher than set max Velocity (did not implement workaround it cus it was unlikely)!\n"; return;}

    double darkFrameTime=darkFrameNum/maxFPS;
    report+=util::toString("Putting ",darkFrameNum," dark frames after end, lasting: ",darkFrameTime," s\n");
    totalFrameNum=readTime*maxFPS;
    report+=util::toString("\nTotal expected number of useful frames for FFT: ",totalFrameNum,"\n");
    int optimalDFTsize = cv::getOptimalDFTSize((int)(readTime*maxFPS));
    report+=util::toString("Optimal number of frames (get as close to this as possible): ",optimalDFTsize,"\n");

    double NLambda=vsConv(range)/vsConv(led_wl)*2;
    i2NLambda=2*NLambda;    //we want it to round down
    report+=util::toString("Total of ",NLambda," wavelengths (for best precision, should be an integer)\n");
    peakLoc=std::nearbyint(totalFrameNum/vsConv(ppwl));
    report+=util::toString("Expecting the peak to be at i=",totalFrameNum/vsConv(ppwl)," in the FFT.\n");
    if(peakLoc+peakLocRange>=totalFrameNum || peakLoc-peakLocRange<=0){report+=util::toString("Error: PeakLoc+-",peakLocRange," should be between 0 and totalFrameNum!\n"); return;}

    PVTmeasure->addAction(XPS::iuScopeLED,true);
    PVTmeasure->add(movTime, 0,0, 0,0, Offset,0, 0,0);
    PVTmeasure->add(readAccelTime,  0,0, 0,0, -readAccelDis,-readVelocity, 0,0);
    PVTmeasure->add(readTime, 0,0, 0,0, -readRangeDis,-readVelocity, 0,0);
    PVTmeasure->addAction(XPS::iuScopeLED,false);
    PVTmeasure->add(readAccelTime,  0,0, 0,0, -readAccelDis,0, 0,0);
    if(darkFrameTime>movTime+readAccelTime)
        PVTmeasure->add(darkFrameTime-movTime-readAccelTime,  0,0, 0,0, 0,0, 0,0);
    PVTmeasure->add(movTime, 0,0, 0,0, Offset,0, 0,0);
    PVTmeasure->addAction(XPS::iuScopeLED,true);
    total_meas_time=2*movTime+2*readAccelTime+readTime+((darkFrameTime>movTime+readAccelTime)?(darkFrameTime-movTime-readAccelTime):0);

    exec_dat ret;
    ret=go.pXPS->verifyPVTobj(PVTmeasure);
    if(ret.retval!=0) {report+=util::toString("Error: Verify PVTmeasure failed, retval was",ret.retstr,"\n"); return;}

    PVTsRdy=true;
}
int NA=0;
void pgScanGUI::doOneRound(char cbAvg_override, char cbTilt_override, char cbRefl_override){
    if(!PVTsRdy) recalculate();
    measurementInProgress=true;
    go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound,this,cbAvg_override, cbTilt_override, cbRefl_override));
}

void pgScanGUI::doNRounds(int N, double redoIfMaskHasMore, int redoN, cv::Rect roi, char cbTilt_override, char cbRefl_override){
    if(!PVTsRdy) recalculate();
    varShareClient<pgScanGUI::scanRes>* scanRes=result.getClient();

    int doneN=0;
    const pgScanGUI::scanRes* res;
    cv::Mat roiMask;
    do{
        measurementInProgress=true;
        go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound,this,-1,cbTilt_override, cbRefl_override));
        while(measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        res=scanRes->get();
        doneN++;
        if(roi.width!=0 && roi.height!=0) roiMask=cv::Mat(res->mask,roi);
        else roiMask=cv::Mat(res->mask);
    }while(cv::countNonZero(roiMask)>roiMask.cols*roiMask.rows*redoIfMaskHasMore && doneN<=redoN);

    while(res->avgNum<N){
        if(MLP._lock_meas.try_lock()){
            go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound,this,1,cbTilt_override, cbRefl_override));
            MLP._lock_meas.unlock();
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        res=scanRes->get();
    }
    delete scanRes;
    while(!MLP._lock_meas.try_lock()) {QCoreApplication::processEvents(QEventLoop::AllEvents, 10); std::this_thread::sleep_for(std::chrono::milliseconds(100));}
    MLP._lock_meas.unlock();
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

void pgScanGUI::_correctTilt(scanRes* res){
    int nRows=res->depth.rows;
    int nCols=res->depth.cols;
    cv::Mat blured;
    double sigma=tiltCorBlur->val;
    int ksize=sigma*5;
    if(!(ksize%2)) ksize++;
    //cv::GaussianBlur(res->depth, blured, cv::Size(ksize, ksize), sigma, sigma);   //using just gaussian blur also blurs out the inf edges maksed over, which sometimes causes tilt to fail
    cv::bilateralFilter(res->depth, blured, ksize, sigma, sigma);                   //this does not blur over obvious sharp edges (but is a bit slower)
    double derDbound=tiltCorThrs->val;
    cv::Mat firstD, secondD, mask, maskT;                                           cv::Mat mask4hist;
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

    if(findBaseline->val){      //we also want to find the baseline level: we only look at points that are flat enough (some of the original flat image must be visible)
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

void pgScanGUI::_doOneRound(char cbAvg_override, char cbTilt_override, char cbRefl_override){
    MLP._lock_meas.lock();                       //wait for other measurements to complete
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    if(!PVTsRdy) return;
    scanRes* res=new scanRes;

    int nFrames=totalFrameNum;
    FQ* framequeue;
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);

    go.pXPS->execPVTobj(PVTmeasure, &PVTret);

    for(int i=0;i!=95;i++){
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(total_meas_time*1000/100)));
        MLP.progress_meas=i+1;
        if(PVTret.check_if_done()) break;
    }
    PVTret.block_till_done();
    MLP.progress_meas=100;
    framequeue->setUserFps(0);

    XPS::raxis poss=go.pXPS->getPos(XPS::mgroup_XYZF);
    for(int i=0;i!=3;i++) res->pos[i]=poss.pos[i];

    std::lock_guard<std::mutex>lock(MLP._lock_comp);    // wait till other processing is done
    MLP._lock_meas.unlock();                             // allow new measurement to start

    if(framequeue->getFullNumber()<nFrames) {std::cerr<<"ERROR: took "<<framequeue->getFullNumber()<<" frames, expected at least "<<nFrames<<".\n";go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);return;}
    double mean=0;
    for(int i=0;i!=10;i++)
        mean+= cv::mean(*framequeue->getUserMat(i))[0];
    mean/=10;
    for(int i=0;i!=framequeue->getFullNumber();i++){
        double iMean= cv::mean(*framequeue->getUserMat(framequeue->getFullNumber()-1-i))[0];
        if(iMean<4*mean/5 && framequeue->getFullNumber()>nFrames){
            framequeue->freeUserMat(framequeue->getFullNumber()-1-i);
            i--;
        } else break;
    }
    if(framequeue->getFullNumber()<nFrames) {std::cerr<<"ERROR: after removing dark frames left with "<<framequeue->getFullNumber()<<" frames, expected at least "<<nFrames<<".\n";go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);return;}
    while(framequeue->getFullNumber()>nFrames)
        framequeue->freeUserMat(0);

    int nRows=framequeue->getUserMat(0)->rows;
    int nCols=framequeue->getUserMat(0)->cols;

    cv::Mat mat2D(nFrames, nCols, CV_32F);
    cv::UMat maskUMat(nRows, nCols, CV_8U);
    cv::UMat maskUMatNot(nRows, nCols, CV_8U);

    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
    cv::UMat resultFinalPhase(nCols, nRows, CV_32F);    //rows and cols flipped for convenience, transpose it after main loop!

    int savePixX{-1}, savePixY{-1}; //writing pixel to file: for selected pixel
    std::string savePixFname;
    if(clickDataLock.try_lock()){
        if(savePix){
            if(clickCoordX>=0 && clickCoordX<nCols && clickCoordY>=0 && clickCoordY<nRows){
                savePixX=clickCoordX;
                savePixY=clickCoordY;
                savePixFname=clickFilename;
            }
            savePix=false;
        }
        clickDataLock.unlock();
    }

    cv::UMat refl;
    bool fillRefl=((cbGetRefl->val && cbRefl_override==0) || cbRefl_override==1);
    if(fillRefl) refl=cv::UMat(nRows, nCols, CV_32F);

    MLP.progress_comp=0;
    for(int k=0;k!=nRows;k++){      //Processing row by row
        //first copy the matrices such that time becomes a column
        for(int i=0;i!=nFrames;i++)
            framequeue->getUserMat(i)->row(k).copyTo(mat2D.row(i));
        //sending the matrix to the GPU, transposing and preforming an DFT
        //Umat2D=mat2D.getUMat(cv::ACCESS_READ);
        cv::UMat Umat2D;
        mat2D.copyTo(Umat2D);
        cv::transpose(Umat2D,Umat2D);

        cv::dft(Umat2D, Umat2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
        //Umat2D.copyTo(fft2D); //no need to copy the whole matrix

        //writing pixel to file: for selected pixel
        if(k==savePixY){
            std::ofstream wfile(savePixFname);
            cv::Mat fft2D;
            Umat2D.copyTo(fft2D);
            for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<mat2D.at<float>(i,savePixX)<<" "<<std::abs(fft2D.at<std::complex<float>>(savePixX, i))<<" "<<std::arg(fft2D.at<std::complex<float>>(savePixX, i))<<"\n";
            wfile.close();
        }

        //getting phase and magnitude, phase at the LED wavelength/2 goes to final matrix, magnitude(2*peakLocRange+1 rows) goes for further eval

        cv::UMat magn;
        std::vector<cv::UMat> planes{2};
        cv::split(Umat2D.colRange(peakLoc-peakLocRange,peakLoc+peakLocRange+1), planes);
        cv::magnitude(planes[0], planes[1], magn);
        cv::phase(planes[0].col(peakLocRange), planes[1].col(peakLocRange), resultFinalPhase.col(k));       //only the actual peak

        //checking if the magnitudes of peakLocRange number of points around the LED wavelength/2 are higher than it, if so, measurement is unreliable
        bool first=true;
        cv::multiply((double)dis_thresh->val,magn.col(peakLocRange),magn.col(peakLocRange));
        cv::UMat cmpRes;
        cv::UMat cmpFinRes;
        for(int j=1;j<=peakLocRange;j++){
            if(first) {compare(magn.col(peakLocRange), magn.col(peakLocRange+j), cmpFinRes, cv::CMP_LT); first=false;}
            else{
                compare(magn.col(peakLocRange), magn.col(peakLocRange+j), cmpRes, cv::CMP_LT);
                add(cmpFinRes, cmpRes, cmpFinRes);
            }
            compare(magn.col(peakLocRange), magn.col(peakLocRange-j), cmpRes, cv::CMP_LT);
            add(cmpFinRes, cmpRes, cmpFinRes);
        }
        cmpFinRes=cmpFinRes.reshape(0,1);
        cmpFinRes.copyTo(maskUMat.row(k));

        if(fillRefl){
            cv::split(Umat2D.col(0), planes);
            cv::magnitude(planes[0], planes[1], magn);
            cv::transpose(magn,magn);
            cv::divide(magn,nFrames,magn);
            magn.copyTo(refl.row(k));
        }

        //std::cerr<<"done with dft"<<k<<"\n";
        MLP.progress_comp=60*(k+1)/nRows;
    }
    std::cerr<<"done with dfts\n";
    int dilation_size=exclDill->val;
    if(dilation_size>0){
        cv::Mat element=cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2*dilation_size+1,2*dilation_size+1), cv::Point(dilation_size,dilation_size));
        cv::dilate(maskUMat, maskUMat, element);
    }

    bitwise_not(maskUMat, maskUMatNot);
    maskUMat.copyTo(res->mask);
    maskUMatNot.copyTo(res->maskN);

    cv::transpose(resultFinalPhase,resultFinalPhase);   //now its the same rows,cols as the camera images
    resultFinalPhase.copyTo(res->depth);

    if(getExpMinMax) calcExpMinMax(framequeue, &res->maskN);
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

    if(debugDisplayModeSelect->index==0){   // if debug no unwrap is off
        resultFinalPhase.copyTo(res->depth);
        cv::phase_unwrapping::HistogramPhaseUnwrapping::Params params;
        params.width=nCols;
        params.height=nRows;
        cv::Ptr<cv::phase_unwrapping::HistogramPhaseUnwrapping> phaseUnwrapping = cv::phase_unwrapping::HistogramPhaseUnwrapping::create(params);
        phaseUnwrapping->unwrapPhaseMap(res->depth, res->depth,res->maskN);    //unfortunately this does not work for UMat (segfaults for ocv 4.1.2), thats why we shuffle the mat back and forth
        res->depth.copyTo(resultFinalPhase);
    }
    resultFinalPhase.convertTo(resultFinalPhase,-1,vsConv(led_wl)/4/M_PI);

    MLP.progress_comp=90;

    resultFinalPhase.setTo(std::numeric_limits<float>::max() , maskUMat);
    resultFinalPhase.copyTo(res->depth);
    if(useCorr->try_lock()){
        if(cor==nullptr)std::cerr<<"Correction was nullptr!\n";
        else if(cor->depth.empty())std::cerr<<"Correction depthmap was empty!\n";
        else if(res->depth.cols==cor->depth.cols && res->depth.rows==cor->depth.rows) cv::subtract(res->depth,cor->depth,res->depth,res->maskN);
        else std::cerr<<"Cannot do correction: the measured and correction depthmaps are of different dimensions!\n";
        useCorr->unlock();
    }

    if(pgMGUI==nullptr) res->XYnmppx=0;
    else res->XYnmppx=pgMGUI->getNmPPx();
    res->avgNum=1;

    if(bSaveAvgMess){  // for autosaving raw data - for debug and bookeeping purposes - for saving individual measurements that are being averaged.
        saveScan(res,util::toString(stringSaveAvgMess,'/',saveIter),true);
        saveIter++;
    }

    if((cbAvg->val && cbAvg_override==0) || cbAvg_override==1){
        varShareClient<pgScanGUI::scanRes>* temp=result.getClient();
        const scanRes* oldScanRes=temp->get();
        if(oldScanRes==nullptr);
        else if(oldScanRes->pos[0]!=res->pos[0] || oldScanRes->pos[1]!=res->pos[1] || oldScanRes->XYnmppx!=res->XYnmppx || oldScanRes->depth.cols!=nCols || oldScanRes->depth.rows!=nRows); //we moved, no point to make average
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
                if(MLP._lock_meas.try_lock()){MLP._lock_meas.unlock();measurementInProgress=false;}
                delete res;
                return;
            }

            res->mask.setTo(255,oldScanRes->mask);
            bitwise_not(res->mask, res->maskN);
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

    if((cbCorrectTilt->val && cbTilt_override==0) || cbTilt_override==1) _correctTilt(res);     //autocorrect tilt
    else {res->tiltCor[0]=0; res->tiltCor[1]=0;}

    cv::minMaxLoc(res->depth, &res->min, &res->max, nullptr, nullptr, res->maskN);  //the ignored mask values will be <min , everything is in nm

    if(fillRefl){
        refl.convertTo(res->refl,CV_8U,2,-127.5);
    }

    result.put(res);

    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"Calculation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
    std::cerr<<"done nr "<<NA<<"\n"; NA++;
    std::cerr<<"Free: "<<go.pGCAM->iuScope->FQsPCcam.getFreeNumber()<<"\n";
    std::cerr<<"Full: "<<go.pGCAM->iuScope->FQsPCcam.getFullNumber()<<"\n";

    MLP.progress_comp=100;
    if(MLP._lock_meas.try_lock()){MLP._lock_meas.unlock();measurementInProgress=false;}
}

void pgScanGUI::calcExpMinMax(FQ* framequeue, cv::Mat* mask){
    int minn=256,maxx=-1;
    for(int i=0;i!=framequeue->getFullNumber();i++){
        double min,max;
        cv::minMaxLoc(*framequeue->getUserMat(i), &min, &max, 0, 0, *mask);
        if(min<minn)minn=min;
        if(max>maxx)maxx=max;
    }
    Q_EMIT doneExpMinmax(minn, maxx);
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
    double min, max; cv::Point ignore;
    cv::minMaxLoc(cv::Mat(scan->depth, roi), &min, &max, &ignore, &ignore, cv::Mat(scan->maskN, roi));
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
    double min, max; cv::Point ignore;
    cv::minMaxLoc(cv::Mat(scan->depth, roi), &min, &max, &ignore, &ignore, cv::Mat(scan->maskN, roi));
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
    if(scan0->depth.cols!=scan1->depth.cols || scan0->depth.rows!=scan1->depth.rows) {std::cerr<<"difScans got matrices with nonequal dimensions\n"; return ret;}

    cv::Mat depth0, depth1, mask0, mask1;
    if(xDifShift->value()!=0 || yDifShift->value()!=0){
        float xShft, yShft;
        int nCols=scan0->depth.cols;    float xShiftFrac=modf(xDifShift->value(), &xShft);
        int nRows=scan0->depth.rows;    float yShiftFrac=modf(yDifShift->value(), &yShft);

        if(abs(xShft)>=nCols || abs(yShft)>=nRows) {std::cerr<<"difScans specified shift larger than mat dimensions\n"; return ret;}
        depth0=scan0->depth(cv::Rect(xShft>0?0:abs(xShft), yShft>0?0:abs(yShft), nCols-abs(xShft), nRows-abs(yShft)));
        mask0 =scan0->mask (cv::Rect(xShft>0?0:abs(xShft), yShft>0?0:abs(yShft), nCols-abs(xShft), nRows-abs(yShft)));
        depth1=scan1->depth(cv::Rect(xShft>0?abs(xShft):0, yShft>0?abs(yShft):0, nCols-abs(xShft), nRows-abs(yShft)));
        mask1 =scan1->mask (cv::Rect(xShft>0?abs(xShft):0, yShft>0?abs(yShft):0, nCols-abs(xShft), nRows-abs(yShft)));

        if(xShiftFrac!=0 || yShiftFrac!=0){
            cv::Mat trans=(cv::Mat_<double>(2,3) << 1, 0, xShiftFrac, 0, 1, yShiftFrac);
            cv::warpAffine(depth0, depth0, trans, depth0.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
            cv::dilate(mask0,mask0,cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3,3), cv::Point(1,1)));
            cv::Rect cut(cv::Rect(xShiftFrac>0?1:0, yShiftFrac>0?1:0, nCols-abs(xShft)-1, nRows-abs(yShft)-1));
            depth0=depth0(cut); mask0 =mask0 (cut);
            depth1=depth1(cut); mask1 =mask1 (cut);
        }
    }else{
        depth0=scan0->depth; mask0=scan0->mask;
        depth1=scan1->depth; mask1=scan1->mask;
    }

    cv::subtract(depth1, depth0, ret.depth);
    int nCols=depth0.cols;
    int nRows=depth0.rows;
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
    cv::Point ignore;
    cv::minMaxLoc(ret.depth, &ret.min, &ret.max, &ignore, &ignore, ret.maskN);
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
