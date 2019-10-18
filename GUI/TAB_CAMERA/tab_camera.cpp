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
        ppwl = new val_selector(20., "tab_camera_ppwl", "Points Per Wavelength: ", 4., 2000.,  &changed);
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
        LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
        framequeueDisp->freeUserMat();
    }

}

void tab_camera::updatePVTs(std::string &report){
    PVTsRdy=false;
    for(int i=0;i!=2;i++){
        PVTtoPos[i]->clear();
        PVTmeasure[i]->clear();
    }

    double minFPS,maxFPS;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &maxFPS);
    double readTime=vsConv(range)*vsConv(ppwl)/vsConv(led_wl)/maxFPS;   //s
    double readRangeDis=vsConv(range)/1000000;
    double readVelocity=readRangeDis*readTime;                          //mm/s
    report+=util::toString("Read Time =",readTime," s\nRead Range Distance:",readRangeDis," mm\nRead Velocity:",readVelocity," m/s\n");
    if(readVelocity>vsConv(max_vel)) {
        report+="Read Velocity is higher than set max Velocity!\n";
        return;
    }
    double readAccelTime=readVelocity/vsConv(max_acc);
    double readAccelDis=vsConv(max_acc)/2*readAccelTime*readAccelTime;
    double newOffset=readRangeDis/2+readAccelDis;
    if(newOffset!=setOffset) getCentered();
    setOffset=newOffset;

    double movTime=2*sqrt(2*newOffset/vsConv(max_acc));
    double movMaxVelocity=vsConv(max_acc)*movTime;

    report+=util::toString("Read Acceleration Time :",readAccelTime," s\nRead Acceleration Distance:",readAccelDis," mm\nNew Offset:",newOffset," mm\nMovement Time:",movTime," s\nMovement Max Velocity:",movMaxVelocity," m/s\n");
    if(movMaxVelocity > vsConv(max_vel)){
        report+="Max move Velocity is higher than set max Velocity (did not implement workaround it cus it was unlikely)!\n";
        return;
    }

    double darkFrameTime=darkFrameNum/maxFPS;
    report+=util::toString("Putting ",darkFrameNum," dark frames after end, lasting: ",darkFrameTime," s\n");
    totalFrameNum=readTime*maxFPS;
    report+=util::toString("\nTotal expected number of useful frames for FFT: ",totalFrameNum,"\n");
    int optimalDFTsize = cv::getOptimalDFTSize((int)(readTime*maxFPS));
    report+=util::toString("Optimal number of frames (get as close to this as possible): ",optimalDFTsize,"\n");

    double NLambda=vsConv(range)/vsConv(led_wl);
    report+=util::toString("Total of ",NLambda," wavelengths (for best precision, should be an integer)\n");

    for(int i=0;i!=2;i++){
        int s=i?-1:1;
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
            report+=util::toString("Verify PVTtoPos failed, retval was",ret.retstr,"\n");
            return;
        }
        ret=go.pXPS->verifyPVTobj(PVTmeasure[i]);
        if(ret.retval!=0) {
            report+=util::toString("Verify PVTmeasure failed, retval was",ret.retstr,"\n");
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
            framequeue->freeUserMat(i);
            std::cerr<<"Frame "<<(framequeue->getFullNumber()-1-i)<<" freed.\n";
        } else break;
    }
    while(framequeue->getFullNumber()>nFrames){
        framequeue->freeUserMat(0);
        std::cerr<<"Frame 0 freed.\n";
    }
    std::cerr<<"got "<<framequeue->getFullNumber()<<" frames.\n";

    int nRows=framequeue->getUserMat(0)->rows;
    int nCols=framequeue->getUserMat(0)->cols;
    cv::UMat Umat2D;
    cv::UMat Ufft2D;
    cv::Mat mat2D(nFrames, nCols, CV_32F, cv::Scalar::all(0));
    cv::Mat fft2D;
    cv::Mat mat2DDepth(nRows, nCols, CV_32F, cv::Scalar::all(0));
    for(int k=0;k!=nRows;k++){
        for(int i=0;i!=nFrames;i++)
            framequeue->getUserMat(i)->row(k).copyTo(mat2D.row(i));
        cv::transpose(mat2D,mat2D);

        if(k==500){
            std::ofstream wfile ("onepixtest.txt");
            for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<mat2D.at<float>(500,i)<<"\n";
            wfile.close();
        }

        Umat2D=mat2D.getUMat(cv::ACCESS_READ);
        cv::dft(Umat2D, Ufft2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
            //fft2D=Ufft2D.getMat(cv::ACCESS_READ);     //this wont work as its asynchronous
        Ufft2D.copyTo(fft2D);

        std::cerr<<"done with dft"<<k<<"\n";
        mat2D=mat2D.reshape(0,mat2D.cols);              //the header is now wrong for next iteration, but we dont use transpose here, because we overwrite the data later anyway, and reshape just changes the header
    }

    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
    procDone=true;                          //signal that the processing is done
}
