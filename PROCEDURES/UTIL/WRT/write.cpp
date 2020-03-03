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
    imgUmPPx=new val_selector(10, "pgWrite_imgUmPPx", "Scaling: ", 0.1, 100, 3, 0, {"um/Px"});
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
    constC=settingWdg[index]->constC;
    pointSpacing=settingWdg[index]->pointSpacing;
}
writeSettings::writeSettings(uint num, pgWrite* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    focus=new val_selector(0, util::toString("writeSettings_focus",num), "Focus (in relation to green center)", -1000, 1000, 3, 0, {"um"});
    slayout->addWidget(focus);
    duration=new val_selector(1, util::toString("writeSettings_duration",num), "Pulse duration", 0.001, 1000, 3, 0, {"ms"});
    slayout->addWidget(focus);
    constA=new val_selector(1, util::toString("writeSettings_constA",num), "Constant A", 0, 1, 6, 0, {"nm"});
    slayout->addWidget(constA);
    constB=new val_selector(100, util::toString("writeSettings_constB",num), "Constant B", 0, 10000, 3);
    slayout->addWidget(constB);
    constX0=new val_selector(1000, util::toString("writeSettings_constI00",num), "Constant I\u2080\u2080", 0, 8192, 3, 0, {"a.u."});
    slayout->addWidget(constX0);
    constC=new val_selector(0, util::toString("writeSettings_constC",num), "Constant C", 0, 100, 3, 0, {"1/nm"});
    slayout->addWidget(constC);
    pointSpacing=new val_selector(1, util::toString("writeSettings_pointSpacing",num), "Point spacing", 0.001, 10, 3, 0, {"um"});
    slayout->addWidget(pointSpacing);
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

    bool doNC=constC->val!=0?true:false;        //do neigbour correction (ie overlap. correction)
    int wrInt;
    for(int j=0;j!=ints.rows;j++) for(int i=0;i!=ints.cols;i++){
//        if(!doNC) wrInt=
//        else{

//        }
//        ints.at<uint16_t>(j,i)=0;
    }
}

void pgWrite::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status;
}
void pgWrite::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn || WRImage.empty() || !writeDM->isEnabled()) return;
    double ratio=imgUmPPx->val/pointSpacing->val;
    double xSize=round(ratio*WRImage.cols)*1000/pgMGUI->getNmPPx();
    double ySize=round(ratio*WRImage.rows)*1000/pgMGUI->getNmPPx();
    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, img->rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
