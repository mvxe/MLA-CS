#include "write.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "GUI/tab_monitor.h"    //for debug purposes

pgWrite::pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI, procLockProg& MLP): pgBeAn(pgBeAn), pgMGUI(pgMGUI), MLP(MLP){
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
    alayout->addWidget(new twid(pulse, pulseDur));
    alayout->addWidget(new hline);
    importImg=new QPushButton("Import image");
    connect(importImg, SIGNAL(released()), this, SLOT(onLoadImg()));
    depthMaxval=new val_selector(10, "(if 8/16bit)Maxval=", 0.1, 500, 3, 0, {"nm"});
    conf["depthMaxval"]=depthMaxval;
    alayout->addWidget(new twid(importImg,depthMaxval));
    dTCcor=new val_selector(1, "Pulse duration correction", 0.1, 3, 3);
    conf["dTCcor"]=dTCcor;
    corDTCor=new QPushButton("Correct Correction");
    connect(corDTCor, SIGNAL(released()), this, SLOT(onCorDTCor()));
    alayout->addWidget(new twid(dTCcor,corDTCor));
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
    QLabel* textl=new QLabel("Predicting height change with formula:\n\u0394H=A(\u0394T-\u0394T\u2080\u2080)exp[-B/(\u0394T-\u0394T\u2080\u2080)],\nwhere \u0394T and \u0394T\u2080\u2080 are pulse durations.");
    textl->setWordWrap(true);
    slayout->addWidget(textl);
}

void pgWrite::onCorDTCor(){
    bool ok;
    float preH=QInputDialog::getDouble(gui_activation, "Correct intensity correction", "Input actual written height (nm) for given expected height.", 0.001, 0, 1000, 3, &ok);
    if(!ok) return;
    float cDT=getDT(depthMaxval->val/1000000);
    dTCcor->setValue(1);
    float gDT=getDT(preH/1000000);
    dTCcor->setValue(cDT/gDT);
}
void pgWrite::onCorPPR(){
    bool ok;
    float preH=QInputDialog::getDouble(gui_activation, "Correct plateau-peak ratio", "Input actual written plateau height (nm) for given expected peak height.", 0.001, 0, 1000, 3, &ok);
    if(!ok) return;
    float cDT=getDT(depthMaxval->val/1000000);
    float gDT=getDT(preH/1000000);
    plataeuPeakRatio->setValue(plataeuPeakRatio->val*gDT/cDT);
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
    constA=settingWdg[index]->constA;
    constB=settingWdg[index]->constB;
    constX0=settingWdg[index]->constX0;
    plataeuPeakRatio=settingWdg[index]->plataeuPeakRatio;
    pointSpacing=settingWdg[index]->pointSpacing;
    writeZeros=settingWdg[index]->writeZeros;
}
writeSettings::writeSettings(uint num, pgWrite* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    focus=new val_selector(0, "Focus offset", -1000, 1000, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["focus"]=focus;
    slayout->addWidget(focus);
    focusXcor=new val_selector(0, "Focus X cor.", -1000, 1000, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["focusXcor"]=focusXcor;
    slayout->addWidget(focusXcor);
    focusYcor=new val_selector(0, "Focus Y cor", -1000, 1000, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["focusYcor"]=focusYcor;
    slayout->addWidget(focusYcor);
    constA=new val_selector(100, "Constant A", 0, 100000, 3, 0, {"nm/ms"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["constA"]=constA;
    slayout->addWidget(constA);
    constB=new val_selector(0, "Constant B", 0, 10000, 3, 0, {"ms"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["constB"]=constB;
    slayout->addWidget(constB);
    constX0=new val_selector(0, "Constant \u0394T\u2080\u2080", 0, 1000, 5, 0, {"ms"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["constT00"]=constX0;
    slayout->addWidget(constX0);
    plataeuPeakRatio=new val_selector(1, "Plataeu-Peak height ratio", 1, 10, 3);
    parent->conf[parent->selectWriteSetting->getLabel(num)]["plataeuPeakRatio"]=plataeuPeakRatio;
    corPPR=new QPushButton("Correct PPR");
    slayout->addWidget(new twid(plataeuPeakRatio,corPPR));
    pointSpacing=new val_selector(1, "Point spacing", 0.01, 10, 3, 0, {"um"});
    parent->conf[parent->selectWriteSetting->getLabel(num)]["pointSpacing"]=pointSpacing;
    slayout->addWidget(pointSpacing);
    writeZeros=new checkbox_gs(false,"Write zeros");
    writeZeros->setToolTip("If enabled, areas that do not need extra height will be written at threshold duration anyway (slower writing but can give more consistent zero levels if calibration is a bit off).");
    parent->conf[parent->selectWriteSetting->getLabel(num)]["writeZeros"]=writeZeros;
    slayout->addWidget(writeZeros);
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
void pgWrite::onWriteDM(cv::Mat* override, double override_depthMaxval, double override_imgmmPPx, double override_pointSpacing, double override_focus, double ov_fxcor, double ov_fycor){
    cv::Mat tmpWrite, resizedWrite;
    cv::Mat* src;
    double vdepthMaxval, vimgmmPPx, vpointSpacing, vfocus, vfocusXcor, vfocusYcor;
    if(override!=nullptr){
        src=override;
        vdepthMaxval=override_depthMaxval;
        vimgmmPPx=override_imgmmPPx;
        vpointSpacing=override_pointSpacing;
        vfocus=override_focus;
        vfocusXcor=ov_fxcor;
        vfocusYcor=ov_fycor;
    }
    else{
        src=&WRImage;
        vdepthMaxval=depthMaxval->val/1000000;
        vimgmmPPx=imgUmPPx->val/1000;
        vpointSpacing=pointSpacing->val/1000;
        vfocus=focus->val/1000;
        vfocusXcor=focusXcor->val/1000;
        vfocusYcor=focusYcor->val/1000;
    }

    if(src->type()==CV_8U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/255);
    }else if(src->type()==CV_16U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/65536);
    }else if(src->type()==CV_32F) src->copyTo(tmpWrite);
    else {QMessageBox::critical(gui_activation, "Error", "Image type not compatible.\n"); writeDM->setEnabled(false); return;}

    double ratio=vimgmmPPx/vpointSpacing;
    if(vimgmmPPx<vpointSpacing) cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_AREA);   //shrink
    else cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_CUBIC);                         //enlarge

    // at this point resizedWrite contains the desired depth at each point
    std::lock_guard<std::mutex>lock(MLP._lock_proc);
    if(!go.pRPTY->connected) return;
    pgMGUI->chooseObj(false);    // switch to writing
    pulse_precision=go.pRPTY->getPulsePrecision();  // pulse duration unit

    double offsX,offsY;
    offsX=(resizedWrite.cols-1)*vpointSpacing;
    offsY=(resizedWrite.rows-1)*vpointSpacing;
    CTRL::CO CO(go.pRPTY);
        // move to target and correct for focus (this can happen simultaneously with objective switch)
    pgMGUI->corCOMove(CO,vfocusXcor,vfocusYcor,vfocus);
    pgMGUI->corCOMove(CO,-offsX/2,-offsY/2,0);
        // wait till motion completes
    CO.addHold("X",CTRL::he_motion_ontarget);
    CO.addHold("Y",CTRL::he_motion_ontarget);
    CO.addHold("Z",CTRL::he_motion_ontarget);
    CO.execute();
    CO.clear(true);

    unsigned nops;
    double pulse;
    bool xdir=0;
    for(int j=0;j!=resizedWrite.rows;j++){   // write row by row (so that processing for the next row is done while writing the previous one, and the operation can be terminated more easily)
        for(int i=0;i!=resizedWrite.cols;i++){
            pulse=getDT(resizedWrite.at<float>(resizedWrite.rows-j-1,xdir?(resizedWrite.cols-i-1):i));          // Y inverted because image formats have flipped Y
            if(i!=0) pgMGUI->corCOMove(CO,(xdir?-1:1)*vpointSpacing,0,0);
            if(pulse==0) continue;
            CO.addHold("X",CTRL::he_motion_ontarget);
            CO.addHold("Y",CTRL::he_motion_ontarget);
            CO.addHold("Z",CTRL::he_motion_ontarget);   // because corCOMove may correct Z too
            CO.pulseGPIO("wrLaser",pulse);
        }
        if (j!=resizedWrite.rows-1) pgMGUI->corCOMove(CO,0,vpointSpacing,0);
        CO.execute();
        CO.clear(true);
        xdir^=1;

        while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
        MLP.progress_proc=100./resizedWrite.rows*j;
    }

    pgMGUI->corCOMove(CO,-vfocusXcor,-vfocusYcor,-vfocus);
    pgMGUI->corCOMove(CO,(xdir?-1:1)*offsX/2,-offsY/2,0);
    CO.execute();
    CO.clear(true);

    while(CO.getProgress()<1) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    MLP.progress_proc=100;
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
    onWriteDM(&tagImage,settingWdg[4]->depthMaxval->val/1000000,settingWdg[4]->imgUmPPx->val/1000, settingWdg[4]->pointSpacing->val/1000,settingWdg[4]->focus->val/1000,settingWdg[4]->focusXcor->val/1000,settingWdg[4]->focusYcor->val/1000);
}
void pgWrite::onWriteTag(){
    std::cerr<<"writing tag\n";
    cv::Size size=cv::getTextSize(tagText->text().toStdString(), OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, settingWdg[4]->fontThickness->val, nullptr);
    tagImage=cv::Mat(size.height+4,size.width+4,CV_8U,cv::Scalar(0));
    cv::putText(tagImage,tagText->text().toStdString(), {0,size.height+1}, OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, cv::Scalar(255), settingWdg[4]->fontThickness->val, cv::LINE_AA);
    onWriteDM(&tagImage,settingWdg[4]->depthMaxval->val/1000000,settingWdg[4]->imgUmPPx->val/1000, settingWdg[4]->pointSpacing->val/1000,settingWdg[4]->focus->val/1000,settingWdg[4]->focusXcor->val/1000,settingWdg[4]->focusYcor->val/1000);
}

float pgWrite::getDT(float H, float H0){
    float min=(constX0->val/1000)*dTCcor->val/plataeuPeakRatio->val;
    float ret=1;            // we limit pulse duration to 1 second
    float mid;
    if(H<=H0) return writeZeros->val?min:0;         //its already higher, dont need to write a pulse (if writeZeros is false)
    while(ret-min>pulse_precision){
        mid=(min+ret)/2;
        if(calcH(mid,H0)>H-H0) ret=mid;
        else min=mid;
    }
    return ret;
}
float pgWrite::calcH(float DT, float H0){   // we dont use H0 in this model though
    float DDT=DT-(constX0->val/1000)*dTCcor->val/plataeuPeakRatio->val;
    return (constA->val/1000)/dTCcor->val*DDT*expf(-(constB->val/1000)*dTCcor->val/DDT);
}

//float pgWrite::gaussian(float x, float y, float a, float wx, float wy, float an){
//    float A=powf(cosf(an),2)/2/powf(wx,2)+powf(sinf(an),2)/2/powf(wy,2);
//    float B=sinf(2*an)/2/powf(wx,2)-sinf(2*an)/2/powf(wy,2);
//    float C=powf(sinf(an),2)/2/powf(wx,2)+powf(cosf(an),2)/2/powf(wy,2);
//    return a*expf(-A*powf(x,2)-B*(x)*(y)-C*powf(y,2));
//}


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
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX), img->rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY), xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
