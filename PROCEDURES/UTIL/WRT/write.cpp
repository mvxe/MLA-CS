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
    pulseDur=new val_selector(1, "Dur", 0.001, 10000, 3, 0, {"ms"});
    conf["pulseDur"]=pulseDur;
    alayout->addWidget(new hline);
    alayout->addWidget(new twid(pulse, pulseDur));
    alayout->addWidget(new hline);
    importImg=new QPushButton("Import image");
    connect(importImg, SIGNAL(released()), this, SLOT(onLoadImg()));
    depthMaxval=new val_selector(10, "(if 8/16bit)Maxval=", 0.1, 500, 3, 0, {"nm"});
    conf["depthMaxval"]=depthMaxval;
    alayout->addWidget(new twid(importImg,depthMaxval));
    ICcor=new val_selector(1, "Intensity correction", 0.1, 3, 3);
    conf["ICcor"]=ICcor;
    corICor=new QPushButton("Correct Correction");
    connect(corICor, SIGNAL(released()), this, SLOT(onCorICor()));
    alayout->addWidget(new twid(ICcor,corICor));
    imgUmPPx=new val_selector(10, "Scaling: ", 0.001, 100, 3, 0, {"um/Px"});
    conf["imgUmPPx"]=imgUmPPx;
    alayout->addWidget(imgUmPPx);
    writeDM=new HQPushButton("Write");
    connect(writeDM, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    connect(writeDM, SIGNAL(released()), this, SLOT(onWriteDM()));
    writeFrame=new HQPushButton("Frame");
    connect(writeFrame, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteFrameAreaOn(bool)));
    connect(writeFrame, SIGNAL(released()), this, SLOT(onWriteFrame()));
    alayout->addWidget(new twid(writeDM,writeFrame));
    tagText=new QLineEdit;
    writeTag=new HQPushButton("Write Tag");
    connect(writeTag, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOnTag(bool)));
    connect(writeTag, SIGNAL(released()), this, SLOT(onWriteTag()));
    alayout->addWidget(new twid(new QLabel("Tag:"),tagText,writeTag));

    writeDM->setEnabled(false);
    writeFrame->setEnabled(false);
    alayout->addWidget(new hline);

    selectWriteSetting=new smp_selector("Select write setting: ", 0, {"Set0", "Set1", "Set2", "Set3", "Tag"});    //should have Nset strings
    conf["selectWriteSetting"]=selectWriteSetting;
    slayout->addWidget(selectWriteSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new writeSettings(i, this));
        slayout->addWidget(settingWdg.back());
        connect(settingWdg.back()->corPPR, SIGNAL(released()), this, SLOT(onCorPPR()));
    }
    connect(selectWriteSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    onMenuChange(0);
    QLabel* textl=new QLabel("Predicting height change with formula:\n\u0394H=A(I-I\u2080)exp(-B/(I-I\u2080)),\nwhere I and I\u2080 are intensities.");
    textl->setWordWrap(true);
    slayout->addWidget(textl);

    max_vel=new val_selector(25, "Max velocity: ", 0.001, 25, 3, 0, {"mm/s"});
    conf["max_vel"]=max_vel;
    slayout->addWidget(max_vel);
    max_acc=new val_selector(100, "Max acceleration: ", 0.001, 100, 3, 0, {"mm/s^2"});
    conf["max_acc"]=max_acc;
    slayout->addWidget(max_acc);
}

void pgWrite::onCorICor(){
    bool ok;
    float preH=QInputDialog::getDouble(gui_activation, "Correct intensity correction", "Input actual written plateau height for given expected height.", 0.001, 0, 1000, 3, &ok);
    if(!ok) return;
    uint cInt=getInt(depthMaxval->val);
    ICcor->setValue(1);
    uint gInt=getInt(preH);
    ICcor->setValue((double)cInt/gInt);
}
void pgWrite::onCorPPR(){
    bool ok;
    float preH=QInputDialog::getDouble(gui_activation, "Correct plateau-peak ratio", "Input actual written plateau height for given expected peak height.", 0.001, 0, 1000, 3, &ok);
    if(!ok) return;
    uint cInt=getInt(depthMaxval->val);
    uint gInt=getInt(preH);
    plataeuPeakRatio->setValue(plataeuPeakRatio->val*gInt/cInt);
}
void pgWrite::onPulse(){
    if(!go.pRPTY->connected) return;
    pgMGUI->chooseObj(false);    // switch to writing
    CTRL::CO co(go.pRPTY);
    co.addHold("X",CTRL::he_motion_ontarget);
    co.addHold("Y",CTRL::he_motion_ontarget);
    co.addHold("Z",CTRL::he_motion_ontarget);
    co.pulseGPIO("wrLaser",pulseDur->val/1000);
    co.execute();
}
void pgWrite::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    focus=settingWdg[index]->focus;
    focusXcor=settingWdg[index]->focusXcor;
    focusYcor=settingWdg[index]->focusYcor;
    duration=settingWdg[index]->duration;
    constA=settingWdg[index]->constA;
    constB=settingWdg[index]->constB;
    constX0=settingWdg[index]->constX0;
    plataeuPeakRatio=settingWdg[index]->plataeuPeakRatio;
    pointSpacing=settingWdg[index]->pointSpacing;
}
writeSettings::writeSettings(uint num, pgWrite* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    focus=new val_selector(0, "Focus (in relation to green center)", -1000, 1000, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["focus"]=focus;
    slayout->addWidget(focus);
    focusXcor=new val_selector(0, "Focus X cor.", -1000, 1000, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["focusXcor"]=focusXcor;
    slayout->addWidget(focusXcor);
    focusYcor=new val_selector(0, "Focus Y cor", -1000, 1000, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["focusYcor"]=focusYcor;
    slayout->addWidget(focusYcor);
    duration=new val_selector(1, "Pulse duration", 0.001, 1000, 3, 0, {"ms"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["duration"]=duration;
    slayout->addWidget(duration);
    constA=new val_selector(1, "Constant A", 0, 1, 6, 0, {"nm"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["constA"]=constA;
    slayout->addWidget(constA);
    constB=new val_selector(100, "Constant B", 0, 10000, 3);
    parent->conf[parent->selectWriteSetting->getLabel(num)]["constB"]=constB;
    slayout->addWidget(constB);
    constX0=new val_selector(1000, "Constant I\u2080\u2080", 0, 8192, 3, 0, {"a.u."});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["constI00"]=constX0;
    slayout->addWidget(constX0);
    plataeuPeakRatio=new val_selector(1, "Plataeu-Peak height ratio", 1, 10, 3);
    parent->conf[parent->selectWriteSetting->getLabel(num)]["plataeuPeakRatio"]=plataeuPeakRatio;
    corPPR=new QPushButton("Correct PPR");
    slayout->addWidget(new twid(plataeuPeakRatio,corPPR));
    pointSpacing=new val_selector(1, "Point spacing", 0.01, 10, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["pointSpacing"]=pointSpacing;
    slayout->addWidget(pointSpacing);
    if(num==4){ //tag settings
        fontFace=new smp_selector("Font ", 0, OCV_FF::qslabels());
        parent->conf[parent->selectWriteSetting->getLabel(num)]["fontFace"]=fontFace;
        slayout->addWidget(fontFace);
        fontSize=new val_selector(1., "Font Size ", 0, 10., 2);
        parent->conf[parent->selectWriteSetting->getLabel(num)]["fontSize"]=fontSize;
        slayout->addWidget(fontSize);
        fontThickness=new val_selector(1, "Font Thickness ", 1, 10, 0, 0, {"px"});
        parent->conf[parent->selectWriteSetting->getLabel(num)]["fontThickness"]=fontThickness;
        slayout->addWidget(fontThickness);
        imgUmPPx=new val_selector(10, "Scaling: ", 0.001, 100, 3, 0, {"um/Px"});
        parent->conf[parent->selectWriteSetting->getLabel(num)]["imgUmPPx"]=imgUmPPx;
        slayout->addWidget(imgUmPPx);
        depthMaxval=new val_selector(10, "Maxval=", 0.1, 500, 3, 0, {"nm"});
        parent->conf[parent->selectWriteSetting->getLabel(num)]["depthMaxval"]=depthMaxval;
        slayout->addWidget(depthMaxval);
        frameDis=new val_selector(10, "Frame Distance=", 0.1, 500, 3, 0, {"um"});
        parent->conf[parent->selectWriteSetting->getLabel(num)]["frameDis"]=frameDis;
        slayout->addWidget(frameDis);
    }
}
void pgWrite::onLoadImg(){
    writeDM->setEnabled(false);
    std::string fileName=QFileDialog::getOpenFileName(gui_activation,"Select file containing stucture to write (Should be either 8bit or 16bit image (will be made monochrome if color), or 32bit float.).", "","Images (*.pfm *.png *.jpg *.tif)").toStdString();
    if(fileName.empty()) return;
    WRImage=cv::imread(fileName, cv::IMREAD_GRAYSCALE|cv::IMREAD_ANYDEPTH);
    if(WRImage.empty()) {QMessageBox::critical(gui_activation, "Error", "Image empty.\n"); return;}

    writeDM->setEnabled(true);
    writeFrame->setEnabled(true);
}
void pgWrite::onWriteDM(cv::Mat* override, double override_depthMaxval, double override_imgUmPPx, double override_pointSpacing, double override_duration, double override_focus, double ov_fxcor, double ov_fycor){
//TODO!
    throw std::invalid_argument("onWriteDM still needs to be transitioned to CTRL");
//TODO!
    cv::Mat tmpWrite, resizedWrite;
    cv::Mat* src;
    double vdepthMaxval, vimgUmPPx, vpointSpacing, vduration, vfocus, vfocusXcor, vfocusYcor;
    if(override!=nullptr){
        src=override;
        vdepthMaxval=override_depthMaxval;
        vimgUmPPx=override_imgUmPPx;
        vpointSpacing=override_pointSpacing;
        vduration= override_duration;
        vfocus=override_focus;
        vfocusXcor=ov_fxcor;
        vfocusYcor=ov_fycor;
    }
    else{
        src=&WRImage;
        vdepthMaxval=depthMaxval->val;
        vimgUmPPx=imgUmPPx->val;
        vpointSpacing=pointSpacing->val;
        vduration=duration->val;
        vfocus=focus->val;
        vfocusXcor=focusXcor->val;
        vfocusYcor=focusYcor->val;
    }

    if(src->type()==CV_8U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/255);
    }else if(src->type()==CV_16U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/65536);
    }else if(src->type()==CV_32F) src->copyTo(tmpWrite);
    else {QMessageBox::critical(gui_activation, "Error", "Image type not compatible.\n"); writeDM->setEnabled(false); return;}

    double ratio=vimgUmPPx/vpointSpacing;
    if(vimgUmPPx<vpointSpacing) cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_AREA);   //shrink
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
//TODO!    pgMGUI->corPvt(po,0.1, vfocusXcor/1000, 0, vfocusYcor/1000, 0, 0, 0,vfocus/1000,0);
    commands.push_back(CQF::GPIO_MASK(0x40,0,0));
    commands.push_back(CQF::GPIO_DIR (0x40,0,0));
    commands.push_back(CQF::W4TRIG_GPIO(CQF::HIGH,false,0x40,0x00));
    po->addAction(XPS::writingLaser,true);

    cv::Point2d lastpos{0,0};
    cv::Point2d nextpos;
    double vel;
    double dist;
    double time;    // in s
    double pulseWaitTime=std::ceil(vduration);      //we round up pulse time to n ms, to make sure we dont have any XPS related timing rounding error
    for(int j=0;j!=ints.rows;j++) for(int i=0;i!=ints.cols;i++){
        int ii=(j%2)?i:(ints.cols-i-1);
        if(ints.at<uint16_t>(j,ii)==0) continue;
        nextpos={-round((ii-ints.cols/2.)*vpointSpacing*100)/100,-round((j-ints.rows/2.)*vpointSpacing*100)/100};          //in um, rounding to 0.01um precision (should be good enough for the stages) to avoid rounding error (with relative positions later on, thats why here we round absolute positions)
        dist=cv::norm(lastpos-nextpos)/1000;//in mm
        vel=sqrt(max_acc->val*dist);        // v=sqrt(2as) for half the distance -> max speed at constant acceleration
        if(vel>max_vel->val){               // we must split the motion into three: acceleration -> constant -> deacceleration
            time=2*max_vel->val/max_acc->val + (dist-pow(max_vel->val,2)/max_acc->val)/max_vel->val;       // accel+deaccel+(dist-2*(v^2/2a))
        }else{                              // we must split the motion into two: acceleration -> deacceleration
            time=2*sqrt(dist/max_acc->val); // t=sqrt(2s/a), twice the time (acc and deacc) and half the distance
        }
        time=(ceil(time/servoCycle)*servoCycle);    // we round time up to XPS servo cycle to prevent rounding (relative) errors later
        if(time>0){
//TODO!            pgMGUI->corPvt(po,time, (nextpos.x-lastpos.x)/1000, 0, (nextpos.y-lastpos.y)/1000, 0, 0, 0,0,0);
            commands.push_back(CQF::WAIT(time/8e-9));
        }
//TODO!        pgMGUI->corPvt(po,pulseWaitTime/1000, 0, 0, 0, 0, 0, 0,0,0);
        commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));    //for debugging purposes
        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, ints.at<uint16_t>(j,ii), 0));
        commands.push_back(CQF::WAIT((vduration)/8e-6 - 3));
        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
        if(pulseWaitTime!=vduration) commands.push_back(CQF::WAIT((pulseWaitTime-vduration)/8e-6));
        lastpos=nextpos;
    }
//TODO!    pgMGUI->corPvt(po,0.1, -lastpos.x/1000-vfocusXcor/1000, 0, -lastpos.y/1000-vfocusYcor/1000, 0, 0, 0,-vfocus/1000,0);
    po->addAction(XPS::writingLaser,false);

    if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";go.pXPS->destroyPVTobj(po);return;}
    printf("Used %d of %d commands\n",commands.size(), go.pRPTY->getNum(RPTY::A2F_RSMax,0));
    //if(go.pRPTY->getNum(RPTY::A2F_RSMax,0)<=commands.size()) {std::cerr<<"too many commands\n";go.pXPS->destroyPVTobj(po);return;}
    go.pRPTY->A2F_write(0,commands.data(),commands.size());
    commands.clear();

    go.pXPS->execPVTobj(po, &ret);
    while(!ret.check_if_done())QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);

    //TODO cap point number and reprogram for large number of points to be written
    //TODO add calculation that takes into account measured depth as pre
    go.pXPS->destroyPVTobj(po);
}
void pgWrite::onWriteFrame(){
    std::cerr<<"writing frame\n";
    cv::Size size=WRImage.size();
    double ratio=imgUmPPx->val; double fr=settingWdg[4]->frameDis->val;
    int xSize=round(ratio*WRImage.cols+2*fr)/settingWdg[4]->imgUmPPx->val;
    int ySize=round(ratio*WRImage.rows+2*fr)/settingWdg[4]->imgUmPPx->val;
    tagImage=cv::Mat(ySize,xSize,CV_8U,cv::Scalar(0));
    int tglen=fr/settingWdg[4]->imgUmPPx->val;
    for(int i=0;i!=2;i++) for(int j=0;j<tglen;j++){
        if(j<xSize){
            tagImage.at<uchar>(i*(ySize-1),j        )=255;
            tagImage.at<uchar>(i*(ySize-1),xSize-1-j)=255;
        }
        if(j<ySize){
            tagImage.at<uchar>(j,        i*(xSize-1))=255;
            tagImage.at<uchar>(ySize-1-j,i*(xSize-1))=255;
        }
    }
    onWriteDM(&tagImage,settingWdg[4]->depthMaxval->val,settingWdg[4]->imgUmPPx->val, settingWdg[4]->pointSpacing->val,settingWdg[4]->duration->val,settingWdg[4]->focus->val,settingWdg[4]->focusXcor->val,settingWdg[4]->focusYcor->val);
}
void pgWrite::onWriteTag(){
    std::cerr<<"writing tag\n";
    cv::Size size=cv::getTextSize(tagText->text().toStdString(), OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, settingWdg[4]->fontThickness->val, nullptr);
    tagImage=cv::Mat(size.height+4,size.width+4,CV_8U,cv::Scalar(0));
    cv::putText(tagImage,tagText->text().toStdString(), {0,size.height+1}, OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, cv::Scalar(255), settingWdg[4]->fontThickness->val, cv::LINE_AA);
    onWriteDM(&tagImage,settingWdg[4]->depthMaxval->val,settingWdg[4]->imgUmPPx->val, settingWdg[4]->pointSpacing->val,settingWdg[4]->duration->val,settingWdg[4]->focus->val,settingWdg[4]->focusXcor->val,settingWdg[4]->focusYcor->val);
}

uint pgWrite::getInt(float post, float pre){
    const float precision=0.01;
    float minI=constX0->val*ICcor->val/plataeuPeakRatio->val;
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
    float DInt=Int-constX0->val*ICcor->val/plataeuPeakRatio->val;
    return constA->val/ICcor->val*DInt*expf(-constB->val*ICcor->val/DInt);
}
float pgWrite::gaussian(float x, float y, float a, float wx, float wy, float an){
    float A=powf(cosf(an),2)/2/powf(wx,2)+powf(sinf(an),2)/2/powf(wy,2);
    float B=sinf(2*an)/2/powf(wx,2)-sinf(2*an)/2/powf(wy,2);
    float C=powf(sinf(an),2)/2/powf(wx,2)+powf(cosf(an),2)/2/powf(wy,2);
    return a*expf(-A*powf(x,2)-B*(x)*(y)-C*powf(y,2));
}


void pgWrite::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status?1:0;
}
void pgWrite::onChangeDrawWriteAreaOnTag(bool status){
    drawWriteAreaOn=status?2:0;
}
void pgWrite::onChangeDrawWriteFrameAreaOn(bool status){
    drawWriteAreaOn=status?3:0;
}
void pgWrite::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double ratio;
    double xSize;
    double ySize;

    if(drawWriteAreaOn==3){ //frame
        if(WRImage.empty() || !writeFrame->isEnabled()) return;
        ratio=imgUmPPx->val; double fr=settingWdg[4]->frameDis->val;
        xSize=round(ratio*WRImage.cols+2*fr)*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*WRImage.rows+2*fr)*1000/pgMGUI->getNmPPx();
    }else if(drawWriteAreaOn==2){ //tag
        if(tagText->text().toStdString().empty()) return;
        ratio=settingWdg[4]->imgUmPPx->val;    //TODO change
        cv::Size size=cv::getTextSize(tagText->text().toStdString(), OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, settingWdg[4]->fontThickness->val, nullptr);
        xSize=round(ratio*(size.width+1))*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*(size.height+1))*1000/pgMGUI->getNmPPx();
    }else{
        if(WRImage.empty() || !writeDM->isEnabled()) return;
        ratio=imgUmPPx->val;
        xSize=round(ratio*WRImage.cols)*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*WRImage.rows)*1000/pgMGUI->getNmPPx();
    }

    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, img->rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
