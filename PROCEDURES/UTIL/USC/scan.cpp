#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <opencv2/phase_unwrapping.hpp>

pScan::pScan(){

}
pScan::~pScan(){

}
void pScan::run(){

}



//GUI

pgScanGUI::pgScanGUI(std::mutex &_lock_mes, std::mutex &_lock_comp): _lock_mes(_lock_mes), _lock_comp(_lock_comp) {
    for(int i=0;i!=2;i++){
        PVTtoPos[i] = go.pXPS->createNewPVTobj(XPS::mgroup_XYZF,  util::toString("camera_PVTtoPos",i,".txt").c_str());
        PVTmeasure[i] = go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_PVTmeasure",i,".txt").c_str());
    }
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
}

void pgScanGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QHBoxLayout;
    gui_activation->setLayout(alayout);
    bScanOne=new QPushButton;
    bScanOne->setText("One scan");
    connect(bScanOne, SIGNAL(released()), this, SLOT(onBScanOne()));
    alayout->addWidget(bScanOne);
    bScanContinuous=new QPushButton;
    bScanContinuous->setText("Continuous scan");
    bScanContinuous->setCheckable(true);
    connect(bScanContinuous, SIGNAL(toggled(bool)), this, SLOT(onBScanContinuous(bool)));
    alayout->addWidget(bScanContinuous);
    bCenter=new QPushButton;
    bCenter->setText("ReCenter");
    connect(bCenter, SIGNAL(released()), this, SLOT(onBCenter()));
    alayout->addWidget(bCenter);
}
void pgScanGUI::onBScanOne(){if(roundDone)doOneRound();}
void pgScanGUI::onBScanContinuous(bool status){keepMeasuring=status;if(roundDone)doOneRound();}
void pgScanGUI::onBCenter(){getCentered();}

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
    double newOffset=readRangeDis/2+readAccelDis;
    if(newOffset!=setOffset) getCentered(false);
    setOffset=newOffset;
    PVTsRdy=false;
    for(int i=0;i!=2;i++){
        PVTtoPos[i]->clear();
        PVTmeasure[i]->clear();
    }

    double movTime=2*sqrt(2*newOffset/vsConv(max_acc));
    double movMaxVelocity=vsConv(max_acc)*movTime;

    report+=util::toString("Read Acceleration Time :",readAccelTime," s\nRead Acceleration Distance:",readAccelDis," mm\nNew Offset:",newOffset," mm\nMovement Time:",movTime," s\nMovement Max Velocity:",movMaxVelocity," m/s\n");
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
    for(int i=0;i!=2;i++){
        int s=i?(-1):1;
        PVTtoPos[i]->addAction(XPS::iuScopeLED,true);
        PVTtoPos[i]->add(movTime, 0,0, 0,0, s*newOffset,0, 0,0);

        PVTmeasure[i]->addAction(XPS::iuScopeLED,true);
        PVTmeasure[i]->add(readAccelTime,  0,0, 0,0, s*readAccelDis,s*readVelocity, 0,0);
        PVTmeasure[i]->add(readTime, 0,0, 0,0, s*readRangeDis,s*readVelocity, 0,0);
        PVTmeasure[i]->addAction(XPS::iuScopeLED,false);
        PVTmeasure[i]->add(readAccelTime,  0,0, 0,0, s*readAccelDis,0, 0,0);
        PVTmeasure[i]->add(darkFrameTime,  0,0, 0,0, 0,0, 0,0);
        PVTmeasure[i]->addAction(XPS::iuScopeLED,true);
    }

    exec_dat ret;
    for(int i=0;i!=2;i++){
        ret=go.pXPS->verifyPVTobj(PVTtoPos[i]);
        if(ret.retval!=0) {report+=util::toString("Error: Verify PVTtoPos failed, retval was",ret.retstr,"\n"); return;}
        ret=go.pXPS->verifyPVTobj(PVTmeasure[i]);
        if(ret.retval!=0) {report+=util::toString("Error: Verify PVTmeasure failed, retval was",ret.retstr,"\n"); return;}
    }

    PVTsRdy=true;
}

void pgScanGUI::doOneRound(){
    if(!PVTsRdy) recalculate();
    std::thread proc(&pgScanGUI::_doOneRound, this);
    proc.detach();
}

void pgScanGUI::getCentered(bool lock){
    if(!isOffset) return;
    if(!PVTsRdy) recalculate();
    if(!PVTsRdy) return;
    if(lock) std::lock_guard<std::mutex>lock(_lock_mes);
    isOffset=false;
    go.pXPS->execPVTobj(PVTtoPos[dir], &PVTret);
    PVTret.block_till_done();
    dir=0;
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
    if(!isOffset){
        isOffset=true;
        go.pXPS->execPVTobj(PVTtoPos[dir], &PVTret);
        PVTret.block_till_done();
        dir=dir?0:1;
    }
    _lock_mes.lock();                       //wait for other measurements to complete
    roundDone=false;

    //std::this_thread::sleep_for (std::chrono::milliseconds(1000));
    int nFrames=totalFrameNum;
    FQ* framequeue;
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);
    go.pXPS->execPVTobj(PVTmeasure[dir], &PVTret);
    PVTret.block_till_done();
    framequeue->setUserFps(0);
    dir=dir?0:1;

    roundDone=true;                         //signal that the measurement is done
    _lock_mes.unlock();
    _lock_comp.lock();                      //wait till other processing is done
    procDone=false;
    if(roundDone&&keepMeasuring)doOneRound();

    std::cerr<<"got "<<framequeue->getFullNumber()<<" frames.\n";
    double mean=0;
    for(int i=0;i!=10;i++)
        mean+= cv::mean(*framequeue->getUserMat(i))[0];
    mean/=10;
    std::cerr<<"Most frames mean is "<<mean<<".\n";
    for(int i=0;i!=framequeue->getFullNumber();i++){
        double iMean= cv::mean(*framequeue->getUserMat(framequeue->getFullNumber()-1-i))[0];
        std::cerr<<"Frame "<<(framequeue->getFullNumber()-1-i)<<" mean is "<<iMean<<".\n";
        if(iMean<4*mean/5 && framequeue->getFullNumber()>nFrames){
            std::cerr<<"Frame "<<(framequeue->getFullNumber()-1-i)<<" freed.\n";
            framequeue->freeUserMat(framequeue->getFullNumber()-1-i);
            i--;
        } else break;
    }
    while(framequeue->getFullNumber()>nFrames){
        framequeue->freeUserMat(0);
        std::cerr<<"Frame 0 freed.\n";
    }
    std::cerr<<"got "<<framequeue->getFullNumber()<<" frames.\n";

    int nRows=framequeue->getUserMat(0)->rows;
    int nCols=framequeue->getUserMat(0)->cols;

    cv::Mat mat2D(nFrames, nCols, CV_32F);
    cv::Mat* measured=new cv::Mat(cv::Size(nCols, nRows), CV_8U);

    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
    cv::UMat resultFinalPhase(cv::Size(nRows,nCols), CV_32F);    //rows and cols flipped for convenience, transpose it after main loop!
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
        cv::UMat Umat2D=mat2D.getUMat(cv::ACCESS_READ);
        cv::transpose(Umat2D,Umat2D);
        cv::UMat Ufft2D;
        cv::dft(Umat2D, Ufft2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
        //Ufft2D.copyTo(fft2D); //no need to copy the whole matrix

        //some writing to file as a test: TODO remove
        if(k==500){
            cv::Mat fft2D;
            Ufft2D.copyTo(fft2D);
            std::ofstream wfile ("onepixtestFFT.txt");
            for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<std::abs(fft2D.at<std::complex<float>>(500, i))<<" "<<std::arg(fft2D.at<std::complex<float>>(500, i))<<"\n";
            wfile.close();
        }

        //getting phase and magnitude, phase at the LED wavelength/2 goes to final matrix, magnitude(2*peakLocRange+1 rows) goes for further eval
        cv::UMat magn;
        std::vector<cv::UMat> planes(2);
        cv::split(Ufft2D.colRange(peakLoc-peakLocRange,peakLoc+peakLocRange+1), planes);
        cv::magnitude(planes[0], planes[1], magn);
        cv::phase(planes[0].col(peakLocRange), planes[1].col(peakLocRange), resultFinalPhase.col(k));       //only the actual peak

        //checking if the magnitudes of peakLocRange number of points around the LED wavelength/2 are higher than it, if so, measurement is unreliable
        cv::UMat cmpRes;
        cv::UMat cmpFinRes;
        bool first=true;
        cv::multiply(dis_thresh->val,magn.col(peakLocRange),magn.col(peakLocRange));
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
        cmpFinRes.copyTo(measured->row(k));

        //getting the rough phase shift for unwrapping
        //std::cerr<<"umat2d "<<Umat2D.rows<<" "<<Umat2D.cols<<"\n";
//        cv::UMat aux=resultFinalPhase.col(k);
//        cv::UMat auxExp;
//        cv::divide(totalFrameNum,aux,aux);          //we put background into aux
//        cv::repeat(aux,1,totalFrameNum,auxExp);
//        cv::absdiff(Umat2D, auxExp, Umat2D);
//        cv::reduce(Umat2D, aux, 1, CV_REDUCE_MAX);
//        cv::divide(2,Umat2D,Umat2D);
//        cv::repeat(aux,1,totalFrameNum,auxExp);

        std::cerr<<"done with dft"<<k<<"\n";
    }
    cv::transpose(resultFinalPhase,resultFinalPhase);   //now its the same rows,cols as the camera images
    cv::Mat* resultFinalPhaseL=new cv::Mat;
    resultFinalPhase.copyTo(*resultFinalPhaseL);



    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

    cv::Mat* resultFinalPhaseUW=new cv::Mat;
    cv::phase_unwrapping::HistogramPhaseUnwrapping::Params params;
    params.width=nCols;
    params.height=nRows;
    cv::Ptr<cv::phase_unwrapping::HistogramPhaseUnwrapping> phaseUnwrapping = cv::phase_unwrapping::HistogramPhaseUnwrapping::create(params);

    //matOp::spread<uchar>(measured);
    bitwise_not(*measured, *measured);
    phaseUnwrapping->unwrapPhaseMap(*resultFinalPhaseL, *resultFinalPhaseUW,*measured);

    double min,max; cv::Point ignore;
    cv::minMaxLoc(*resultFinalPhaseUW, &min, &max, &ignore, &ignore, *measured);
    bitwise_not(*measured, *measured);
    resultFinalPhaseUW->convertTo(*resultFinalPhaseUW, CV_32F, ((1<<8)-1)/(max-min),-min*((1<<8)-1)/(max-min));
    //resultFinalPhaseUW->setTo(0,*measured);   //no need for this
    std::cerr<<"min,max= "<<min<<" "<<max<<"\n";
    cv::minMaxLoc(*resultFinalPhaseUW, &min, &max);
    std::cerr<<"min,max= "<<min<<" "<<max<<"\n";

    resultFinalPhaseUW->convertTo(*resultFinalPhaseUW, CV_8U, 1);
    cv::minMaxLoc(*resultFinalPhaseUW, &min, &max);
    std::cerr<<"min,max= "<<min<<" "<<max<<"\n";

    resultFinalPhaseL->convertTo(*resultFinalPhaseL, CV_8U, ((1<<8)-1)/M_PI/2);
    measuredM.putMat(measured);
    measuredP.putMat(resultFinalPhaseL);
    measuredPU.putMat(resultFinalPhaseUW);

    procDone=true;                          //signal that the processing is done
    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
    _lock_comp.unlock();
}
