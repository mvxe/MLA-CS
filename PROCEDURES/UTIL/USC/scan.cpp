#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <opencv2/phase_unwrapping.hpp>
#include <opencv2/core/ocl.hpp>

pScan::pScan(){

}
pScan::~pScan(){

}
void pScan::run(){

}



//GUI

pgScanGUI::pgScanGUI(std::mutex& _lock_mes, std::mutex& _lock_comp): _lock_mes(_lock_mes), _lock_comp(_lock_comp) {
    PVTmeasure=go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_PVTmeasure.txt").c_str());
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
    timerCM = new QTimer(this);
    timerCM->setInterval(timerCM_delay);
    connect(timerCM, SIGNAL(timeout()), this, SLOT(onBScanOne()));
}

void pgScanGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QHBoxLayout;
    gui_activation->setLayout(alayout);
    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    bScanOne=new QPushButton;
    bScanOne->setText("One scan");
    connect(bScanOne, SIGNAL(released()), this, SLOT(onBScanOne()));
    tlay->addWidget(bScanOne);
    bScanContinuous=new QPushButton;
    bScanContinuous->setText("Continuous scan");
    bScanContinuous->setCheckable(true);
    connect(bScanContinuous, SIGNAL(toggled(bool)), this, SLOT(onBScanContinuous(bool)));
    tlay->addWidget(bScanContinuous); tlay->addStretch(0); tlay->setMargin(0);
    alayout->addWidget(twid);
    cbCorrectTilt=new QCheckBox("Correct Tilt");
    cbCorrectTilt->setChecked(correctTilt);
    alayout->addWidget(cbCorrectTilt);
    connect(cbCorrectTilt, SIGNAL(toggled(bool)),this, SLOT(setCorrectTilt(bool)));
    alayout->addStretch(0);
}
void pgScanGUI::onBScanContinuous(bool status){if(status)timerCM->start(); else timerCM->stop();}
void pgScanGUI::onBScanOne(){if(_lock_mes.try_lock()){_lock_mes.unlock();doOneRound();}}

void pgScanGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    led_wl=new val_selector(470., "tab_camera_led_wl", "LED Wavelength:", 0.001, 2000., 2, 0, {"nm","um"});
    connect(led_wl, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(led_wl);
    coh_len=new val_selector(20., "tab_camera_coh_len", "Coherence Length L:", 1., 2000., 2, 2, {"nm","um",QChar(0x03BB)});
    connect(coh_len, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(coh_len);
    range=new val_selector(10., "tab_camera_range", "Scan Range:", 1., 2000., 3, 3 , {"nm","um",QChar(0x03BB),"L"});
    connect(range, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(range);
    ppwl=new val_selector(20., "tab_camera_ppwl", "Points Per Wavelength: ", 6, 2000., 2);
    connect(ppwl, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(ppwl);
    max_vel=new val_selector(300., "tab_camera_max_vel", "UScope stage max velocity: ", 1e-9, 300., 2, 0, {"mm/s"});
    connect(max_vel, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(max_vel);
    max_acc=new val_selector(2500., "tab_camera_max_acc", "UScope stage max acceleration: ", 1e-9, 2500., 2, 0, {"mm/s^2"});
    connect(max_acc, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(max_acc);
    dis_thresh=new val_selector(0.9, "tab_camera_dis_thresh", "Peak Discard Threshold: ", 0.1, 1, 2);
    slayout->addWidget(dis_thresh);
    calcL=new QLabel;
    slayout->addWidget(calcL);
    debugDisplayModeSelect=new smp_selector("Display mode select (for debugging purposes): ", 0, {"Unwrapped", "Wrapped"});
    slayout->addWidget(debugDisplayModeSelect);
}

void pgScanGUI::recalculate() {
    if(_lock_mes.try_lock()){
        if(_lock_comp.try_lock()){
            std::string report;
            updatePVTs(report);
            calcL->setText(report.c_str());
            _lock_comp.unlock();
        }
        else timer->start();
        _lock_mes.unlock();
    }
    else timer->start();
}

void pgScanGUI::updatePVTs(std::string &report){
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
    PVTmeasure->add(movTime, 0,0, 0,0, -Offset,0, 0,0);
    PVTmeasure->add(readAccelTime,  0,0, 0,0, readAccelDis,readVelocity, 0,0);
    PVTmeasure->add(readTime, 0,0, 0,0, readRangeDis,readVelocity, 0,0);
    PVTmeasure->addAction(XPS::iuScopeLED,false);
    PVTmeasure->add(readAccelTime,  0,0, 0,0, readAccelDis,0, 0,0);
    if(darkFrameTime>movTime+readAccelTime)
        PVTmeasure->add(darkFrameTime-movTime-readAccelTime,  0,0, 0,0, 0,0, 0,0);
    PVTmeasure->add(movTime, 0,0, 0,0, -Offset,0, 0,0);
    PVTmeasure->addAction(XPS::iuScopeLED,true);

    exec_dat ret;
    ret=go.pXPS->verifyPVTobj(PVTmeasure);
    if(ret.retval!=0) {report+=util::toString("Error: Verify PVTmeasure failed, retval was",ret.retstr,"\n"); return;}

    PVTsRdy=true;
}
int NA=0;
void pgScanGUI::doOneRound(){
    if(!PVTsRdy) recalculate();
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
    if(!PVTsRdy) return;
    _lock_mes.lock();                       //wait for other measurements to complete

    int nFrames=totalFrameNum;
    FQ* framequeue;
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);

    go.pXPS->execPVTobj(PVTmeasure, &PVTret);
    PVTret.block_till_done();
    framequeue->setUserFps(0);

    std::lock_guard<std::mutex>lock(_lock_comp);    // wait till other processing is done
    _lock_mes.unlock();                             // allow new measurement to start

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
    for(int k=0;k!=nRows;k++){      //Processing row by row
        //first copy the matrices such that time becomes a column
        for(int i=0;i!=nFrames;i++)
            framequeue->getUserMat(i)->row(k).copyTo(mat2D.row(i));

        //some writing to file as a test: TODO remove
        if(k==500){
            std::ofstream wfile ("onepixtest.txt");
            for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<mat2D.at<float>(i,500)<<"\n";
            wfile.close();
        }

        //sending the matrix to the GPU, transposing and preforming an DFT
        //Umat2D=mat2D.getUMat(cv::ACCESS_READ);
        cv::UMat Umat2D;
        mat2D.copyTo(Umat2D);
        cv::transpose(Umat2D,Umat2D);

        cv::dft(Umat2D, Umat2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
        //Umat2D.copyTo(fft2D); //no need to copy the whole matrix

        //some writing to file as a test: TODO remove
        if(k==500){
            cv::Mat fft2D;
            Umat2D.copyTo(fft2D);
            std::ofstream wfile ("onepixtestFFT.txt");
            for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<std::abs(fft2D.at<std::complex<float>>(500, i))<<" "<<std::arg(fft2D.at<std::complex<float>>(500, i))<<"\n";
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
    }
    std::cerr<<"done with dfts\n";
    bitwise_not(maskUMat, maskUMatNot);
    cv::Mat* maskMat=new cv::Mat;
    cv::Mat* maskMatNot=new cv::Mat;
    maskUMat.copyTo(*maskMat);
    maskUMatNot.copyTo(*maskMatNot);

    cv::transpose(resultFinalPhase,resultFinalPhase);   //now its the same rows,cols as the camera images
    cv::Mat* resultFinalPhaseL=new cv::Mat;
    resultFinalPhase.copyTo(*resultFinalPhaseL);

    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

    if(debugDisplayModeSelect->index==0){   // if debug no unwrap is off
        resultFinalPhase.copyTo(*resultFinalPhaseL);
        cv::phase_unwrapping::HistogramPhaseUnwrapping::Params params;
        params.width=nCols;
        params.height=nRows;
        cv::Ptr<cv::phase_unwrapping::HistogramPhaseUnwrapping> phaseUnwrapping = cv::phase_unwrapping::HistogramPhaseUnwrapping::create(params);
        phaseUnwrapping->unwrapPhaseMap(*resultFinalPhaseL, *resultFinalPhaseL,*maskMatNot);    //unfortunately this does not work for UMat (segfaults for ocv 4.1.2), thats why we shuffle the mat back and forth
        resultFinalPhaseL->copyTo(resultFinalPhase);
    }
    double min,max; cv::Point ignore;
    resultFinalPhase.convertTo(resultFinalPhase,-1,vsConv(led_wl)/4/M_PI);

    if(correctTilt){ //autocorrect tilt
        cv::UMat padded;
        int m=cv::getOptimalDFTSize(resultFinalPhase.rows);
        int n=cv::getOptimalDFTSize(resultFinalPhase.cols);
        cv::copyMakeBorder(resultFinalPhase, padded, 0, m-resultFinalPhase.rows, 0, n-resultFinalPhase.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));
        cv::UMat dftRes;
        padded.setTo(cv::Scalar::all(0), maskUMat);
        cv::dft(padded, dftRes);
        cv::Mat res;
        cv::UMat(dftRes,{0,0,2,2}).copyTo(res);
        double phiX=M_PI/2-atan(std::abs(res.at<std::complex<float>>(0,1))/nRows/nCols/nCols*2*M_PI);
        double phiY=M_PI/2-atan(std::abs(res.at<std::complex<float>>(1,0))/nRows/nCols/nRows*2*M_PI);
        std::cout<<"Phases X,Y: "<<phiX<< " "<<phiY<<"\n";

        cv::Mat slopeX1(1, nCols, CV_32F);
        cv::UMat slopeUX1(1, nCols, CV_32F);
        cv::UMat slopeUX(nRows, nCols, CV_32F);
        for(int i=0;i!=nCols;i++) slopeX1.at<float>(i)=i*tan(M_PI/2-phiX);
        slopeX1.copyTo(slopeUX1);
        cv::repeat(slopeUX1, nRows, 1, slopeUX);
        cv::add(slopeUX,resultFinalPhase,resultFinalPhase);

        cv::Mat slopeY1(nRows, 1, CV_32F);
        cv::UMat slopeUY1(nRows, 1, CV_32F);
        cv::UMat slopeUY(nRows, nCols, CV_32F);
        for(int i=0;i!=nRows;i++) slopeY1.at<float>(i)=i*tan(M_PI/2-phiY);
        slopeY1.copyTo(slopeUY1);
        cv::repeat(slopeUY1, 1, nCols, slopeUY);
        cv::add(slopeUY,resultFinalPhase,resultFinalPhase);
    }

    cv::minMaxLoc(resultFinalPhase, &min, &max, &ignore, &ignore, maskUMatNot);  //the ignored mask values will be <min , everything is in nm
    resultFinalPhase.copyTo(*resultFinalPhaseL);


//    if(true) for(int j=0;j!=101;j++){ //autocorrect tilt
//        double phiXX=M_PI/2/100*j;
//        cv::Mat slopeX1(1, nCols, CV_32F);
//        cv::UMat slopeUX1(1, nCols, CV_32F);
//        cv::UMat slopeUX(nRows, nCols, CV_32F);
//        const double nmPPx=500;
//        for(int i=0;i!=nCols;i++) slopeX1.at<float>(i)=nmPPx*i*tan(M_PI/2-phiXX);
//        slopeX1.copyTo(slopeUX1);
//        cv::repeat(slopeUX1, nRows, 1, slopeUX);
//        cv::UMat padded;
//        cv::dft(slopeUX, padded);
//        cv::Mat res;
//        cv::UMat(padded,{0,0,2,2}).copyTo(res);
//        double phiX=std::abs(res.at<std::complex<float>>(1,0));
//        double phiY=std::abs(res.at<std::complex<float>>(0,1));
//        std::cout<<"angle, amplitudes X,Y, angles Y: "<<phiXX<< " "<<phiX/nRows/nCols<< " "<<M_PI/2-atan(phiY/nRows/nCols/nmPPx/nCols*2*M_PI)<<"\n";
//    }

    mask.putMat(maskMat);
    maskN.putMat(maskMatNot);
    scanRes.putMat(resultFinalPhaseL,min,max);

    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
    std::cerr<<"done nr "<<NA<<"\n"; NA++;
    std::cerr<<"Free: "<<go.pGCAM->iuScope->FQsPCcam.getFreeNumber()<<"\n";
    std::cerr<<"Full: "<<go.pGCAM->iuScope->FQsPCcam.getFullNumber()<<"\n";

}
