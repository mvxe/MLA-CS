#include "write.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "GUI/tab_monitor.h"    //for debug purposes

pgWrite::pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI): pgBeAn(pgBeAn), pgMGUI(pgMGUI){
    gui_activation=new QWidget;
    gui_settings=new QWidget;

    alayout=new QVBoxLayout;
    slayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    gui_settings->setLayout(slayout);

    pulse=new QPushButton("Pulse");
    connect(pulse, SIGNAL(released()), this, SLOT(onPulse()));
    pulseInt=new val_selector(1000, "pgWrite_pulseInt", "Int:", 1, 8192, 0);
    pulseDur=new val_selector(1, "pgWrite_pulseDur", "Dur", 0.001, 10000, 3, 0, {"ms"});
    alayout->addWidget(new hline);
    alayout->addWidget(new twid(pulse, pulseInt, pulseDur));
    alayout->addWidget(new hline);
    importImg=new QPushButton("Import image");
    connect(importImg, SIGNAL(released()), this, SLOT(onLoadImg()));
    depthMaxval=new val_selector(10, "pgWrite_depthMaxval", "(if 8/16bit)Maxval=", 0.1, 500, 3, 0, {"nm"});
    alayout->addWidget(new twid(importImg,depthMaxval));
    ICcor=new val_selector(1, "pgWrite_ICcor", "Intensity correction", 0.1, 3, 3);
    alayout->addWidget(ICcor);
    imgUmPPx=new val_selector(10, "pgWrite_imgUmPPx", "Scaling: ", 0.001, 100, 3, 0, {"um/Px"});
    alayout->addWidget(imgUmPPx);
    writeDM=new HQPushButton("Write");
    connect(writeDM, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    connect(writeDM, SIGNAL(released()), this, SLOT(onWriteDM()));
    alayout->addWidget(new twid(writeDM));
    writeDM->setEnabled(false);
    alayout->addWidget(new hline);

    selectWriteSetting=new smp_selector("Select write setting: ", 0, {"Set0", "Set1", "Set2", "Set3", "Set4"});    //should have Nset strings
    slayout->addWidget(selectWriteSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new writeSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    connect(selectWriteSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    onMenuChange(0);
    QLabel* textl=new QLabel("Predicting height change with formula:\n\u0394H=A(I-I\u2080)exp(-B/(I-I\u2080)),\nwhere I\u2080=I\u2080\u2080+C\u00B7H\u2080,\nwhere H\u2080 is the height before the write, and I and I\u2080 are intensities.\nSet C to 0 if no pre-height correction is desired.");
    textl->setWordWrap(true);
    slayout->addWidget(textl);

    max_vel=new val_selector(25, "pgWrite_max_vel", "Max velocity: ", 0.001, 25, 3, 0, {"mm/s"});
    slayout->addWidget(max_vel);
    max_acc=new val_selector(100, "pgWrite_max_acc", "Max acceleration: ", 0.001, 100, 3, 0, {"mm/s^2"});
    slayout->addWidget(max_acc);
}

void pgWrite::onPulse(){
    if(!go.pRPTY->connected) return;
    std::vector<uint32_t> commands;
    commands.push_back(CQF::W4TRIG_INTR());
    commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));
    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, pulseInt->val, 0));
    long int dur=pulseDur->val/8e-6-1;
    while(dur>100000000) {commands.push_back(CQF::WAIT(100000000)); dur-=100000000;}
    commands.push_back(CQF::WAIT(dur));
    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
    go.pRPTY->A2F_write(0,commands.data(),commands.size());
    go.pRPTY->trig(1<<0);
}
void pgWrite::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    focus=settingWdg[index]->focus;
    duration=settingWdg[index]->duration;
    constA=settingWdg[index]->constA;
    constB=settingWdg[index]->constB;
    constX0=settingWdg[index]->constX0;
    pointSpacing=settingWdg[index]->pointSpacing;
    FWHMX=settingWdg[index]->FWHMX;
    FWHMY=settingWdg[index]->FWHMY;
    FWHMXYan=settingWdg[index]->FWHMXYan;
}
writeSettings::writeSettings(uint num, pgWrite* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    focus=new val_selector(0, util::toString("writeSettings_focus",num), "Focus (in relation to green center)", -1000, 1000, 3, 0, {"um"});
    slayout->addWidget(focus);
    duration=new val_selector(1, util::toString("writeSettings_duration",num), "Pulse duration", 0.001, 1000, 3, 0, {"ms"});
    slayout->addWidget(duration);
    constA=new val_selector(1, util::toString("writeSettings_constA",num), "Constant A", 0, 1, 6, 0, {"nm"});
    slayout->addWidget(constA);
    constB=new val_selector(100, util::toString("writeSettings_constB",num), "Constant B", 0, 10000, 3);
    slayout->addWidget(constB);
    constX0=new val_selector(1000, util::toString("writeSettings_constI00",num), "Constant I\u2080\u2080", 0, 8192, 3, 0, {"a.u."});
    slayout->addWidget(constX0);
    pointSpacing=new val_selector(1, util::toString("writeSettings_pointSpacing",num), "Point spacing", 0.01, 10, 3, 0, {"um"});
    slayout->addWidget(pointSpacing);
    FWHMX=new val_selector(5, util::toString("writeSettings_FWHMX",num), "FWHM X", 0.1, 100, 2, 0, {"um"});
    slayout->addWidget(FWHMX);
    FWHMY=new val_selector(5, util::toString("writeSettings_FWHMY",num), "FWHM Y", 0.1, 100, 2, 0, {"um"});
    slayout->addWidget(FWHMY);
    FWHMXYan=new val_selector(0, util::toString("writeSettings_FWHMXYan",num), "FWHM XY angle", 0, M_PI, 2, 0, {"rad"});
    slayout->addWidget(FWHMXYan);
}
void pgWrite::onLoadImg(){
    writeDM->setEnabled(false);
    std::string fileName=QFileDialog::getOpenFileName(gui_activation,"Select file containing stucture to write (Should be either 8bit or 16bit image (will be made monochrome if color), or 32bit float.).", "","Images (*.pfm *.png *.jpg *.tif)").toStdString();
    if(fileName.empty()) return;
    WRImage=cv::imread(fileName, cv::IMREAD_GRAYSCALE|cv::IMREAD_ANYDEPTH);
    if(WRImage.empty()) {QMessageBox::critical(gui_activation, "Error", "Image empty.\n"); return;}

    writeDM->setEnabled(true);
}
void pgWrite::onWriteDM(){
    cv::Mat tmpWrite, resizedWrite;
    if(WRImage.type()==CV_8U){
        WRImage.convertTo(tmpWrite, CV_32F, depthMaxval->val/255);
    }else if(WRImage.type()==CV_16U){
        WRImage.convertTo(tmpWrite, CV_32F, depthMaxval->val/65536);
    }else if(WRImage.type()==CV_32F) WRImage.copyTo(tmpWrite);
    else {QMessageBox::critical(gui_activation, "Error", "Image type not compatible.\n"); writeDM->setEnabled(false); return;}

    double ratio=imgUmPPx->val/pointSpacing->val;
    if(imgUmPPx->val<pointSpacing->val) cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_AREA);   //shrink
    else cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_CUBIC);                                 //enlarge

    //now convert depths to a matrix of intensities;
    cv::Mat ints(resizedWrite.rows,resizedWrite.cols,CV_16U);

    for(int j=0;j!=ints.rows;j++) for(int i=0;i!=ints.cols;i++){
        ints.at<uint16_t>(j,i)=getInt(resizedWrite.at<float>(j,i));
    }

    if(!go.pRPTY->connected) return;
    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->setGPIO(XPS::writingLaser,false);

    exec_ret ret;
    PVTobj* po=go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, "pgWrite.txt");
    std::vector<uint32_t> commands;
    //pgMGUI->corPvt(po,0.1, 0, 0, 0, 0, 0, 0,focus->val/1000,0);
    po->add(0.1, 0, 0, 0, 0, 0, 0,focus->val/1000,0);
    commands.push_back(CQF::GPIO_MASK(0x40,0,0));
    commands.push_back(CQF::GPIO_DIR (0x40,0,0));
    commands.push_back(CQF::W4TRIG_GPIO(CQF::HIGH,false,0x40,0x00));
    po->addAction(XPS::writingLaser,true);

    cv::Point2d lastpos{0,0};
    cv::Point2d nextpos;
    double vel;
    double dist;
    double time;    // in s
    double pulseWaitTime=std::ceil(duration->val);      //we round up pulse time to n ms, to make sure we dont have any XPS related timing rounding error
    for(int j=0;j!=ints.rows;j++) for(int i=0;i!=ints.cols;i++){
        if(ints.at<uint16_t>(j,i)==0) continue;
        nextpos={-round((i-ints.cols/2.)*pointSpacing->val*100)/100,-round((j-ints.rows/2.)*pointSpacing->val*100)/100};          //in um, rounding to 0.01um precision (should be good enough for the stages) to avoid rounding error (with relative positions later on, thats why here we round absolute positions)
        dist=cv::norm(lastpos-nextpos)/1000;//in mm
        vel=sqrt(max_acc->val*dist);        // v=sqrt(2as) for half the distance -> max speed at constant acceleration
        if(vel>max_vel->val){               // we must split the motion into three: acceleration -> constant -> deacceleration
            time=2*max_vel->val/max_acc->val + (dist-pow(max_vel->val,2)/max_acc->val)/max_vel->val;       // accel+deaccel+(dist-2*(v^2/2a))
        }else{                              // we must split the motion into two: acceleration -> deacceleration
            time=2*sqrt(dist/max_acc->val); // t=sqrt(2s/a), twice the time (acc and deacc) and half the distance
        }
        time=(ceil(time/servoCycle)*servoCycle);    // we round time up to XPS servo cycle to prevent rounding (relative) errors later
        //std::cerr<<time<<"s "<<pulseWaitTime/1000<<"s "<<dist<<"mm "<<(nextpos.x-lastpos.x)/1000<<"mm "<<(nextpos.y-lastpos.y)/1000<<"mm\n";
        //pgMGUI->corPvt(po,time, (nextpos.x-lastpos.x)/1000, 0, (nextpos.y-lastpos.y)/1000, 0, 0, 0,0,0);
        if(time>0){
            po->add(time, (nextpos.x-lastpos.x)/1000, 0, (nextpos.y-lastpos.y)/1000, 0, 0, 0,0,0);
            commands.push_back(CQF::WAIT(time/8e-9));
        }
        //pgMGUI->corPvt(po,pulseWaitTime/1000, 0, 0, 0, 0, 0, 0,0,0);
        po->add(pulseWaitTime/1000, 0, 0, 0, 0, 0, 0,0,0);
        commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));    //for debugging purposes
        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, ints.at<uint16_t>(j,i), 0));
        commands.push_back(CQF::WAIT((duration->val)/8e-6 - 3));
        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
        if(pulseWaitTime!=duration->val) commands.push_back(CQF::WAIT((pulseWaitTime-duration->val)/8e-6));
        lastpos=nextpos;
    }
    //pgMGUI->corPvt(po,0.1, 0, 0, 0, 0, 0, 0,-focus->val/1000,0);
    po->add(0.1, -lastpos.x/1000, 0, -lastpos.y/1000, 0, 0, 0,-focus->val/1000,0);
    po->addAction(XPS::writingLaser,false);

    if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";go.pXPS->destroyPVTobj(po);return;}
    printf("Used %d of %d commands\n",commands.size(), go.pRPTY->getNum(RPTY::A2F_RSMax,0));
    if(go.pRPTY->getNum(RPTY::A2F_RSMax,0)<=commands.size()) {std::cerr<<"too many commands\n";go.pXPS->destroyPVTobj(po);return;}
    go.pRPTY->A2F_write(0,commands.data(),commands.size());
    commands.clear();

    go.pXPS->execPVTobj(po, &ret);
    while(!ret.check_if_done())QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);

    //TODO cap point number and reprogram for large number of points to be written
    //TODO add calculation that takes into account measured depth as pre
    go.pXPS->destroyPVTobj(po);
}

uint pgWrite::getInt(float post, float pre){
    const float precision=0.01;
    float minI=constX0->val*ICcor->val;
    float retI=8192;
    float mid;
    if(0>=post/*-pre*/) return 0;  //its already higher, dont need to write a pulse
    while(retI-minI>precision){
        mid=(minI+retI)/2;
        if(calcH(mid,pre)>post-pre) retI=mid;
        else minI=mid;
    }
    return roundf(retI);
}
float pgWrite::calcH(float Int, float pre){
    float DInt=Int-constX0->val*ICcor->val;
    return constA->val/ICcor->val*DInt*expf(-constB->val*ICcor->val/DInt);
}
float pgWrite::gaussian(float x, float y, float a, float wx, float wy, float an){
    float A=powf(cosf(an),2)/2/powf(wx,2)+powf(sinf(an),2)/2/powf(wy,2);
    float B=sinf(2*an)/2/powf(wx,2)-sinf(2*an)/2/powf(wy,2);
    float C=powf(sinf(an),2)/2/powf(wx,2)+powf(cosf(an),2)/2/powf(wy,2);
    return a*expf(-A*powf(x,2)-B*(x)*(y)-C*powf(y,2));
}


void pgWrite::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status;
}
void pgWrite::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn || WRImage.empty() || !writeDM->isEnabled()) return;
    double ratio=imgUmPPx->val;
    double xSize=round(ratio*WRImage.cols)*1000/pgMGUI->getNmPPx();
    double ySize=round(ratio*WRImage.rows)*1000/pgMGUI->getNmPPx();
    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, img->rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
