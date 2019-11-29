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
    bScanContinuous->setText("Repeating scan");
    connect(bScanContinuous, SIGNAL(toggled(bool)), this, SLOT(onBScanContinuous(bool)));
    gui_activation->addWidget(bScan);
    gui_activation->addWidget(bScanContinuous);
    cbCorrectTilt=new QCheckBox("Correct Tilt");
    cbCorrectTilt->setChecked(correctTilt);
    gui_activation->addWidget(cbCorrectTilt);
    connect(cbCorrectTilt, SIGNAL(toggled(bool)),this, SLOT(setCorrectTilt(bool)));
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
    slayout->addWidget(selectScanSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new scanSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    connect(selectScanSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    debugDisplayModeSelect=new smp_selector("Display mode select (for debugging purposes): ", 0, {"Unwrapped", "Wrapped"});
    slayout->addWidget(debugDisplayModeSelect);
    onMenuChange(0);
}

scanSettings::scanSettings(uint num, pgScanGUI* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    led_wl=new val_selector(470., util::toString("tab_camera_led_wl",num), "LED Wavelength:", 0.001, 2000., 2, 0, {"nm","um"});
    connect(led_wl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(led_wl);
    coh_len=new val_selector(20., util::toString("tab_camera_coh_len",num), "Coherence Length L:", 1., 2000., 2, 2, {"nm","um",QChar(0x03BB)});
    connect(coh_len, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(coh_len);
    range=new val_selector(10., util::toString("tab_camera_range",num), "Scan Range:", 1., 2000., 3, 3 , {"nm","um",QChar(0x03BB),"L"});
    connect(range, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(range);
    ppwl=new val_selector(20., util::toString("tab_camera_ppwl",num), "Points Per Wavelength: ", 6, 2000., 2);
    connect(ppwl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(ppwl);
    max_vel=new val_selector(300., util::toString("tab_camera_max_vel",num), "UScope stage max velocity: ", 1e-9, 300., 2, 0, {"mm/s"});
    connect(max_vel, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(max_vel);
    max_acc=new val_selector(2500., util::toString("tab_camera_max_acc",num), "UScope stage max acceleration: ", 1e-9, 2500., 2, 0, {"mm/s^2"});
    connect(max_acc, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(max_acc);
    dis_thresh=new val_selector(0.9, util::toString("tab_camera_dis_thresh",num), "Peak Discard Threshold: ", 0.1, 1, 2);
    slayout->addWidget(dis_thresh);
    calcL=new QLabel;
    slayout->addWidget(calcL);
    exclDill=new val_selector(0, util::toString("tab_camera_exclDill",num), "Excluded dillation: ", 0, 100, 0, 0, {"px"});
    slayout->addWidget(exclDill);
    tiltCorBlur=new val_selector(2, util::toString("tab_camera_tiltCorBlur",num), "Tilt Correction Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
    slayout->addWidget(tiltCorBlur);
    tiltCorThrs=new val_selector(0.2, util::toString("tab_camera_tiltCorThrs",num), "Tilt Correction 2nd Derivative Exclusion Threshold: ", 0, 1, 2);
    slayout->addWidget(tiltCorThrs);
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
void pgScanGUI::doOneRound(){
    if(!PVTsRdy) recalculate();
    measurementInProgress=true;
    go.OCL_threadpool.doJob(std::bind(&pgScanGUI::_doOneRound,this));
}

double pgScanGUI::vsConv(val_selector* vs){
    switch(vs->index){
    case 0: return vs->val;
    case 1: return vs->val*1000;
    case 2: return vs->val*vsConv(led_wl);
    case 3: return vs->val*vsConv(coh_len);
    }
}

void pgScanGUI::_doOneRound(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    if(!PVTsRdy) return;
    scanRes* res=new scanRes;
    MLP._lock_meas.lock();                       //wait for other measurements to complete

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
        cv::multiply(dis_thresh->val,magn.col(peakLocRange),magn.col(peakLocRange));
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
    cv::Point ignore;
    resultFinalPhase.convertTo(resultFinalPhase,-1,vsConv(led_wl)/4/M_PI);

    MLP.progress_comp=90;

    if(correctTilt){ //autocorrect tilt
        cv::UMat blured;
        double sigma=tiltCorBlur->val;
        int ksize=sigma*5;
        if(!(ksize%2)) ksize++;
        cv::GaussianBlur(resultFinalPhase, blured, cv::Size(ksize, ksize), sigma, sigma);
        double derDbound=tiltCorThrs->val;
        cv::UMat firstD, secondD, mask, maskT;
        cv::Sobel(blured, firstD, CV_32F,1,0); //X
        cv::Sobel(blured, secondD, CV_32F,2,0); //X
        cv::compare(secondD,  derDbound, mask , cv::CMP_LT);
        cv::compare(secondD, -derDbound, maskT, cv::CMP_LE);
        mask.setTo(cv::Scalar::all(0), maskT);
        mask.setTo(cv::Scalar::all(0), maskUMat);
        res->tiltCor[0]=-cv::mean(firstD,mask)[0]/8;
        cv::Sobel(blured, firstD, CV_32F,0,1); //Y
        cv::Sobel(blured, secondD, CV_32F,2,0); //X
        cv::compare(secondD,  derDbound, mask , cv::CMP_LT);
        cv::compare(secondD, -derDbound, maskT, cv::CMP_LE);
        mask.setTo(cv::Scalar::all(0), maskT);
        mask.setTo(cv::Scalar::all(0), maskUMat);
        res->tiltCor[1]=-cv::mean(firstD,mask)[0]/8;
        std::cout<<"CPhases X,Y: "<<res->tiltCor[0]<< " "<<res->tiltCor[1]<<"\n";


        cv::Mat slopeX1(1, nCols, CV_32F);
        cv::UMat slopeUX1(1, nCols, CV_32F);
        cv::UMat slopeUX(nRows, nCols, CV_32F);
        for(int i=0;i!=nCols;i++) slopeX1.at<float>(i)=i*res->tiltCor[0];
        slopeX1.copyTo(slopeUX1);
        cv::repeat(slopeUX1, nRows, 1, slopeUX);
        cv::add(slopeUX,resultFinalPhase,resultFinalPhase);

        cv::Mat slopeY1(nRows, 1, CV_32F);
        cv::UMat slopeUY1(nRows, 1, CV_32F);
        cv::UMat slopeUY(nRows, nCols, CV_32F);
        for(int i=0;i!=nRows;i++) slopeY1.at<float>(i)=i*res->tiltCor[1];
        slopeY1.copyTo(slopeUY1);
        cv::repeat(slopeUY1, 1, nCols, slopeUY);
        cv::add(slopeUY,resultFinalPhase,resultFinalPhase);
    } else {res->tiltCor[0]=0; res->tiltCor[1]=0;}

    cv::minMaxLoc(resultFinalPhase, &res->min, &res->max, &ignore, &ignore, maskUMatNot);  //the ignored mask values will be <min , everything is in nm
    resultFinalPhase.setTo(std::numeric_limits<float>::max() , maskUMat);
    resultFinalPhase.copyTo(res->depth);

    if(pgMGUI==nullptr) res->XYnmppx=0;
    else res->XYnmppx=pgMGUI->getNmPPx();
    result.put(res);

    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
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
void pgScanGUI::saveScan(const scanRes* scan, std::string fileName){
    pgScanGUI::saveScan(scan, {0,0,scan->depth.cols,scan->depth.rows}, fileName);
}
void pgScanGUI::saveScan(const scanRes* scan, const cv::Rect &roi, std::string fileName){
    if(scan==nullptr) return;
    if(fileName=="") fileName=QFileDialog::getSaveFileName(parent,"Select file for saving Depth Map (raw, float).", "","Images (*.pfm)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".pfm")==std::string::npos) fileName+=".pfm";

    cv::Mat display(scan->depth, roi);
    cv::imwrite(fileName, display);
    std::ofstream wfile(fileName, std::ofstream::app);
    wfile.write(reinterpret_cast<const char*>(&(scan->min)),sizeof(scan->min));
    wfile.write(reinterpret_cast<const char*>(&(scan->max)),sizeof(scan->max));
    wfile.write(reinterpret_cast<const char*>(scan->tiltCor),sizeof(scan->tiltCor));
    wfile.write(reinterpret_cast<const char*>(scan->pos),sizeof(scan->pos));
    wfile.write(reinterpret_cast<const char*>(&(scan->XYnmppx)),sizeof(scan->XYnmppx));
    wfile.close();
}
bool pgScanGUI::loadScan(scanRes* scan, std::string fileName){
    if(scan==nullptr) scan=new scanRes;
    if(fileName=="") fileName=QFileDialog::getOpenFileName(parent,"Select file to load Depth Map (raw, float).", "","Images (*.pfm)").toStdString();
    if(fileName.empty())return false;
    scan->depth=cv::imread(fileName, cv::IMREAD_UNCHANGED);
    cv::compare(scan->depth, std::numeric_limits<float>::max(), scan->mask, cv::CMP_EQ);
    cv::bitwise_not(scan->mask, scan->maskN);
    std::ifstream rfile(fileName);
    int size=sizeof(scan->min)+sizeof(scan->max)+sizeof(scan->tiltCor)+sizeof(scan->pos)+sizeof(scan->XYnmppx);
    rfile.seekg(-size, rfile.end);
    rfile.read(reinterpret_cast<char*>(&(scan->min)),sizeof(scan->min));
    rfile.read(reinterpret_cast<char*>(&(scan->max)),sizeof(scan->max));
    rfile.read(reinterpret_cast<char*>(scan->tiltCor),sizeof(scan->tiltCor));
    rfile.read(reinterpret_cast<char*>(scan->pos),sizeof(scan->pos));
    rfile.read(reinterpret_cast<char*>(&(scan->XYnmppx)),sizeof(scan->XYnmppx));
    rfile.close();
    return true;
}
