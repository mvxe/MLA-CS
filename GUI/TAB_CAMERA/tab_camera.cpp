#include "tab_camera.h"
#include "GUI/mainwindow.h"
#include "includes.h"


tab_camera::tab_camera(QWidget* parent){
    layout=new QHBoxLayout;
    parent->setLayout(layout);

    LDisplay = new mtlabel;
    LDisplay->setMouseTracking(false);
    LDisplay->setFrameShape(QFrame::Box);
    LDisplay->setFrameShadow(QFrame::Plain);
    LDisplay->setScaledContents(false);
    LDisplay->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    TWCtrl = new QTabWidget;
    layout->addWidget(LDisplay);
    layout->addWidget(TWCtrl);
    TWCtrl->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

    pageMotion = new QWidget;
    pageWriting = new QWidget;
    pageSettings = new QWidget;
    TWCtrl->addTab(pageMotion,"Motion");
        motionSettings=new QVBoxLayout;
        pageMotion->setLayout(motionSettings);
        scanOne = new QPushButton;
        motionSettings->addWidget(scanOne);
        scanOne->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        scanOne->setText("Scan One");
        connect(scanOne, SIGNAL(released()), this, SLOT(onScanOneReleased()));
        motionSettings->addStretch(0);

    TWCtrl->addTab(pageWriting,"Writing");

    TWCtrl->addTab(pageSettings,"Settings");
        layoutSettings=new QVBoxLayout;
        pageSettings->setLayout(layoutSettings);
        led_wl = new val_selector(470., "tab_camera_led_wl", "LED Wavelength:", 0.001, 2000., 0, {"nm","um"}, &changed);
        layoutSettings->addWidget(led_wl);
        coh_len = new val_selector(20., "tab_camera_coh_len", "Coherence Length L:", 1., 2000., 2, {"nm","um",QChar(0x03BB)}, &changed);
        layoutSettings->addWidget(coh_len);
        range = new val_selector(10., "tab_camera_range", "Scan Range:", 1., 2000., 3 , {"nm","um",QChar(0x03BB),"L"}, &changed);
        layoutSettings->addWidget(range);
        ppwl = new val_selector(20., "tab_camera_ppwl", "Points Per Wavelength: ", 6, 2000.,  &changed);
        layoutSettings->addWidget(ppwl);
        max_vel = new val_selector(300., "tab_camera_max_vel", "UScope stage max velocity: ", 1e-9, 300., 0, {"mm/s"}, &changed);
        layoutSettings->addWidget(max_vel);
        max_acc = new val_selector(2500., "tab_camera_max_acc", "UScope stage max acceleration: ", 1e-9, 2500., 0, {"mm/s^2"}, &changed);
        layoutSettings->addWidget(max_acc);
        calcL=new QLabel;
        layoutSettings->addWidget(calcL);
        layoutSettings->addStretch(0);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));

    for(int i=0;i!=2;i++){
        PVTtoPos[i] = go.pXPS->createNewPVTobj(XPS::mgroup_XYZF,  util::toString("camera_PVTtoPos",i,".txt").c_str());
        PVTmeasure[i] = go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_PVTmeasure",i,".txt").c_str());
    }
}
tab_camera::~tab_camera(){
    getCentered();
    while(!procDone || !roundDone);
}

void tab_camera::work_fun(){
    if(changed==true){
        changed=false;
        std::string report;
        updatePVTs(report);
        calcL->setText(report.c_str());
    }

    onDisplay=framequeueDisp->getUserMat();
    if(onDisplay!=nullptr){
        {std::lock_guard<std::mutex>lock(measuredMLock);
            if(!measuredM.empty()) {
                if(olay!=nullptr) delete olay;
                olay=measuredM.front();
                measuredM.pop_front();
            }
        }

        if(olay!=nullptr){
            cv::Mat disp=onDisplay->clone();
            cv::cvtColor(disp, disp, CV_GRAY2BGR);
            disp.setTo(cv::Scalar(255,0,0), *olay);
            LDisplay->setPixmap(QPixmap::fromImage(QImage(disp.data, disp.cols, disp.rows, disp.step, QImage::Format_RGB888)));
        }else{
            LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
        }
        framequeueDisp->freeUserMat();
    }

}

void tab_camera::updatePVTs(std::string &report){
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
    if(newOffset!=setOffset) getCentered();
    setOffset=newOffset;
    PVTsRdy=false;
    for(int i=0;i!=2;i++){
        PVTtoPos[i]->clear();
        PVTmeasure[i]->clear();
    }

    double movTime=2*sqrt(2*newOffset/vsConv(max_acc));
    double movMaxVelocity=vsConv(max_acc)*movTime;

    report+=util::toString("Read Acceleration Time :",readAccelTime," s\nRead Acceleration Distance:",readAccelDis," mm\nNew Offset:",newOffset," mm\nMovement Time:",movTime," s\nMovement Max Velocity:",movMaxVelocity," m/s\n");
    if(movMaxVelocity > vsConv(max_vel)){
        report+="Error: Max move Velocity is higher than set max Velocity (did not implement workaround it cus it was unlikely)!\n";
        return;
    }

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
    if(peakLoc+peakLocRange>=totalFrameNum || peakLoc-peakLocRange<=0){
        report+=util::toString("Error: PeakLoc+-",peakLocRange," should be between 0 and totalFrameNum!\n");
        return;
    }

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
        if(ret.retval!=0) {
            report+=util::toString("Error: Verify PVTtoPos failed, retval was",ret.retstr,"\n");
            return;
        }
        ret=go.pXPS->verifyPVTobj(PVTmeasure[i]);
        if(ret.retval!=0) {
            report+=util::toString("Error: Verify PVTmeasure failed, retval was",ret.retstr,"\n");
            return;
        }
    }

    PVTsRdy=true;
}

void tab_camera::doOneRound(){
    std::thread proc(&tab_camera::_doOneRound, this);
    proc.detach();
    //_doOneRound();
}

void tab_camera::getCentered(){
    while(!roundDone);
    if(!isOffset || !PVTsRdy) return;
    isOffset=false;
    go.pXPS->execPVTobj(PVTtoPos[dir], &PVTret);
    PVTret.block_till_done();
    dir=0;
}


void tab_camera::tab_entered(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    running=true;


    framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeueDisp->setUserFps(30,5);

    timer->start(work_call_time);
}
void tab_camera::tab_exited(){
    if(running==false) return;
    running=false;
    getCentered();
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
    timer->stop();
}
double tab_camera::vsConv(val_selector* vs){
    switch(vs->index){
    case 0: return vs->val;
    case 1: return vs->val*1000;
    case 2: return vs->val*vsConv(led_wl);
    case 3: return vs->val*vsConv(coh_len);
    }
}




void tab_camera::_doOneRound(){
    while(!roundDone); roundDone=false;    //wait for other measurements to complete
    if(!PVTsRdy) return;
    if(!isOffset){
        isOffset=true;
        go.pXPS->execPVTobj(PVTtoPos[dir], &PVTret);
        PVTret.block_till_done();
        dir=dir?0:1;
    }
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
    while(!procDone); procDone=false;       //wait till other processing is done


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
    cv::UMat resultFinalPhase(cv::Size(nRows,nCols), CV_32F);    //rows and cols flipped for convinience, transpose it after main loop!
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
//        cv::UMat means;
//        reduce(Umat2D, means, 1, CV_REDUCE_AVG);
//        cv::multiply(means,-1,means);
//        cv::UMat means2(2, 5, CV_32F, means);
//        cv::add(Umat2D,means,Umat2D);

        std::cerr<<"done with dft"<<k<<"\n";
    }
    cv::transpose(resultFinalPhase,resultFinalPhase);   //now its the same rows,cols as the camera images

    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";

    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

    {std::lock_guard<std::mutex>lock(measuredMLock);
    measuredM.push_back(measured);std::cerr<<"measuredM size: "<<measuredM.size()<<"\n";}

    procDone=true;                          //signal that the processing is done
}
