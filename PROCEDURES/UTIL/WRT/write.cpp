#include "write.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "GUI/tab_monitor.h"    //for debug purposes
#include <time.h>
#include "opencv2/core/utils/filesystem.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <QListView>
#include <QStandardItemModel>

pgWrite::pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI, procLockProg& MLP, pgScanGUI* pgSGUI, overlay& ovl): pgBeAn(pgBeAn), pgMGUI(pgMGUI), MLP(MLP), pgSGUI(pgSGUI), ovl(ovl){
    gui_activation=new QWidget;
    gui_settings=new QWidget;
    scanRes=pgSGUI->result.getClient();

    alayout=new QVBoxLayout;
    slayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    gui_settings->setLayout(slayout);

    pulse=new QPushButton("Pulse");
    connect(pulse, SIGNAL(released()), this, SLOT(onPulse()));
    pulseDur=new val_selector(1, "Duration:", 0.001, 10000, 3, 0, {"ms"});
    conf["pulseDur"]=pulseDur;
    pulseh=new hidCon(pulse);
    pulseh->addWidget(new twid(pulseDur));
    alayout->addWidget(pulseh);

    alayout->addWidget(new hline);

    importImg=new QPushButton("Load image");
    connect(importImg, SIGNAL(released()), this, SLOT(onLoadImg()));
    depthMaxval=new val_selector(10, "(if 8/16bit)Maxval = ", 0.1, 500, 3, 0, {"nm"});
    conf["depthMaxval"]=depthMaxval;
    imgUmPPx=new val_selector(10, "Scaling: ", 0.001, 100, 3, 0, {"um/Px"});
    conf["imgUmPPx"]=imgUmPPx;
    importh=new hidCon(importImg);
    importh->addWidget(depthMaxval);
    importh->addWidget(imgUmPPx);
    alayout->addWidget(importh);

    abort=new QPushButton("Abort");
    abort->setVisible(false);
    connect(abort, SIGNAL(released()), this, SLOT(onAbort()));
    alayout->addWidget(new twid(abort));

    writeDM=new HQPushButton("Write");
    connect(writeDM, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    connect(writeDM, SIGNAL(released()), this, SLOT(onWriteDM()));
    scanB=new HQPushButton("Scan");
    scanB->setEnabled(false);
    connect(scanB, SIGNAL(changed(bool)), this, SLOT(onChangeDrawScanAreaOn(bool)));
    connect(scanB, SIGNAL(released()), this, SLOT(onScan()));
    saveB=new QPushButton("Save");
    saveB->setEnabled(false);
    connect(saveB, SIGNAL(released()), this, SLOT(onSave()));
    alayout->addWidget(new twid(writeDM,scanB,saveB));

    tagText=new QLineEdit;
    writeTag=new HQPushButton("Write Tag");
    connect(writeTag, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOnTag(bool)));
    connect(writeTag, SIGNAL(released()), this, SLOT(onWriteTag()));
    alayout->addWidget(new twid(writeTag,new QLabel("Text:"),tagText));

    tagString=new lineedit_gs("A");
    conf["tagString"]=tagString;
    tagSTwid=new twid(new QLabel("Tag String:"),tagString);
    tagSTwid->setVisible(false);
    connect(tagString, SIGNAL(changed()), this, SLOT(onRecomputeTagString()));
    alayout->addWidget(tagSTwid);
    tagUInt=new val_selector(0, "Tag UInt: ", 0, 999999, 0);
    conf["tagUInt"]=tagUInt;
    tagUInt->setVisible(false);
    connect(tagUInt, SIGNAL(changed()), this, SLOT(onRecomputeTagUInt()));
    guessTagUInt=new QPushButton("Guess");
    connect(guessTagUInt, SIGNAL(released()), this, SLOT(guessAndUpdateNextTagUInt()));
    guessTagUInt->setFlat(true);
    alayout->addWidget(new twid(tagUInt,guessTagUInt));

    notes=new QPushButton("Edit notes");
    conf["notes"]=notestring;
    connect(notes, SIGNAL(released()), this, SLOT(onNotes()));
    addNotes=new checkbox_gs(false,"Append notes to .cfg");
    alayout->addWidget(new twid(notes,addNotes));

    useWriteScheduling=new checkbox_gs(false,"Enable write scheduling");
    connect(useWriteScheduling, SIGNAL(changed(bool)), this, SLOT(onUseWriteScheduling(bool)));
    alayout->addWidget(new twid(useWriteScheduling));
    schedulelw=new QListView;
    schedulelw->setVisible(false);
    alayout->addWidget(schedulelw);

    schedulelw->setDragEnabled(true);
    schedulelw->setDragDropMode(QAbstractItemView::InternalMove);
    schedulelw->setDefaultDropAction(Qt::MoveAction);

    schedulemod=new QStandardItemModel(0, 1);
    schedulelw->setModel(schedulemod);

    dTCcor=new val_selector(1, "Pulse duration correction", 0.1, 3, 3);
    conf["dTCcor"]=dTCcor;
    corDTCor=new QPushButton("Correct Correction");
    connect(corDTCor, SIGNAL(released()), this, SLOT(onCorDTCor()));
    alayout->addWidget(new twid(dTCcor,corDTCor));


    writeDM->setEnabled(false);

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
    slayout->addWidget(new hline());


    folderhcon=new hidCon(new QLabel("Default Folders and naming"));
        write_default_folder=new btnlabel_gs(".");
        conf["write_default_folder"]=write_default_folder;
        connect(write_default_folder, SIGNAL(released()), this, SLOT(on_write_default_folder()));
        folderhcon->addWidget(new QLabel("Write source default folder:"));
        folderhcon->addWidget(write_default_folder);
        scan_default_folder=new btnlabel_gs(".");
        conf["scan_default_folder"]=scan_default_folder;
        connect(scan_default_folder, SIGNAL(released()), this, SLOT(on_scan_default_folder()));
        folderhcon->addWidget(new QLabel("Scan destination default folder:"));
        folderhcon->addWidget(scan_default_folder);
        filenaming=new lineedit_gs("%Y-%m/$T$N-$F-$V");
        conf["filenaming"]=filenaming;
        folderhcon->addWidget(new twid(new QLabel("File naming:"),filenaming));
        std::string tooltip{"Date&Time is in ctime/strftime format (%Y, %m, %d, etc).\n"
                            "Other than that custom symbols are:\n"
                            "$T - string tag\n"
                            "$N - numeric tag\n"
                            "$F - source filename\n"
                            "$V - maxval\n"
                            "$X - scaling"};
        filenaming->setToolTip(QString::fromStdString(tooltip));
        connect(filenaming, SIGNAL(changed()), this, SLOT(onCheckTagString()));
        tagnaming=new lineedit_gs("$T$N");
        tagnaming->setToolTip(QString::fromStdString(tooltip));
        conf["tagnaming"]=tagnaming;
        folderhcon->addWidget(new twid(new QLabel("Tag autofill:"),tagnaming));
        connect(tagnaming, SIGNAL(changed()), this, SLOT(onCheckTagString()));
        numbericTagMinDigits=new val_selector(3, "Minimum number of digits in numeric tag: ", 1, 10, 0);
        conf["numbericTagMinDigits"]=numbericTagMinDigits;
        folderhcon->addWidget(numbericTagMinDigits);


    slayout->addWidget(folderhcon);

    scanExtraBorder=new val_selector(0, "Scan extra margin = ", 0, 1000, 0, 0, {"um","px"});
    scanExtraBorder->setToolTip("Generally a good idea, also may catch tag/border.\nIf this goes beyond viewport, the extra will be clipped.");
    conf["scanExtraBorder"]=scanExtraBorder;
    slayout->addWidget(scanExtraBorder);
    scanRepeatN=new val_selector(1, "Scan number of scans: ", 1, 500, 0);
    conf["scanRepeatN"]=scanRepeatN;
    slayout->addWidget(scanRepeatN);
    switchBack2mirau=new checkbox_gs(false,"Switch back to Mirau objective after writing operations.");
    conf["switchBack2mirau"]=switchBack2mirau;
    slayout->addWidget(switchBack2mirau);
}
pgWrite::~pgWrite(){
    delete scanRes;
}
void pgWrite::onAbort(){
    wabort=true;
}
void pgWrite::onNotes(){
    bool ok;
    QString tmp=QInputDialog::getMultiLineText(gui_activation, tr("QInputDialog::getMultiLineText()"),tr("String to be added to .cfg:"), QString::fromStdString(notestring), &ok);
    if(ok) notestring=tmp.toStdString();
}
void pgWrite::replacePlaceholdersInString(std::string& src){
    if(src.empty()) return;
    std::size_t found;
    if(src.find("%")!=std::string::npos){
        time_t rawtime;
        time(&rawtime);
        char buffer [300];
        strftime(buffer,300,src.c_str(),localtime(&rawtime));
        src.clear();
        src=buffer;
    }
    while(std::isspace(src[src.size()-1])) src.pop_back();
    std::string placeholders[5]={"$T","$N","$F","$V","$X"};
    std::string replacements[5]={tagString->text().toStdString(),std::to_string(static_cast<int>(tagUInt->val)),fileName,std::to_string(lastDepth),std::to_string(imgUmPPx->val)};
    for(int i:{3,4}) if(replacements[i].find(".")!=std::string::npos) while(replacements[i].back()=='0') replacements[i].pop_back();    //remove extra zeroes
    found=replacements[2].find_last_of("/");
    if(found!=std::string::npos) replacements[2].erase(0,found+1);
    found=replacements[2].find_last_of(".");
    if(found!=std::string::npos) replacements[2].erase(found,replacements[2].size()-found);
    while(replacements[1].size()<numbericTagMinDigits->val) replacements[1].insert(0,"0");
    for(int i=0;i!=5;i++) while(1){
        found=src.find(placeholders[i]);
        if(found==std::string::npos) break;
        stripDollarSigns(replacements[i]);
        src.replace(found,2,replacements[i]);
    }
}
void pgWrite::stripDollarSigns(std::string &str){
    while(1){
        std::size_t found=str.find("$");
        if(found==std::string::npos) return;
        str.erase(found,1);
    }
}
void pgWrite::onCheckTagString(){
    bool show;
    if(filenaming->get().find("$T")!=std::string::npos) show=true;
    else if(tagnaming->get().find("$T")!=std::string::npos) show=true;
    else show=false;
    tagSTwid->setVisible(show);

    if(filenaming->get().find("$N")!=std::string::npos) show=true;
    else if(tagnaming->get().find("$N")!=std::string::npos) show=true;
    else show=false;
    tagUInt->setVisible(show);
    onRecomputeTag();
}
void pgWrite::onRecomputeTagString(){
    if(tagnaming->get().find("$T")!=std::string::npos) onRecomputeTag();
}
void pgWrite::onRecomputeTagUInt(){
    if(tagnaming->get().find("$N")!=std::string::npos) onRecomputeTag();
}
void pgWrite::onRecomputeTag(){
    std::string ttext=tagnaming->get();
    if(!ttext.empty()){
        replacePlaceholdersInString(ttext);
        tagText->setText(QString::fromStdString(ttext));
    }
}
void pgWrite::guessAndUpdateNextTagUInt(){
    std::vector<std::string> folders;
    std::vector<std::string> files;
    folders.push_back(scan_default_folder->get());
    std::string folder; DIR *wp;
    std::string curFile;
    struct dirent *entry;
    struct stat filetype;
    int newTagN=-1;
    while(!folders.empty()){
        folder=folders.back();
        folders.pop_back();
        wp=opendir(folder.c_str());
        if(wp!=nullptr) while((entry=readdir(wp))){
            curFile=entry->d_name;
            if (curFile!="." && curFile!=".."){
                curFile=folder+'/'+entry->d_name;
                stat(curFile.data(),&filetype);
                if(filetype.st_mode&S_IFDIR) folders.push_back(curFile);
                else if (filetype.st_mode&S_IFREG){
                    if(curFile.size()>3) if(curFile.substr(curFile.size()-4,4).compare(".cfg")==0){
                        std::ifstream ifile(curFile);
                        std::string line;
                        bool right_tag=false;
                        int num=-1;
                        while(!ifile.eof()){
                            std::getline(ifile,line);
                            if(line.find("Tag String: ")!=std::string::npos){
                                line.erase(0,sizeof("Tag String: ")-1);
                                while(std::isspace(line.back())) line.pop_back();
                                right_tag=(line.compare(tagString->text().toStdString())==0);
                            }else if (line.find("Tag UInt: ")!=std::string::npos){
                                line.erase(0,sizeof("Tag UInt: ")-1);
                                num=std::stoi(line);
                            }
                        }
                        if(right_tag && num>newTagN) newTagN=num;
                        ifile.close();
                    }
                }
            }
        }
        closedir(wp);
    }
    if(newTagN>-1) tagUInt->setValue(newTagN+1);
}
void pgWrite::onUseWriteScheduling(bool state){
    writeDM->setText(state?"Schedule Write":"Write");
    writeTag->setText(state?"Schedule Write Tag":"Write Tag");
    schedulelw->setVisible(state);
    saveB->setVisible(!state);
    scanB->setVisible(!state);
}
QStandardItem* pgWrite::addScheduleItem(std::string text, bool toTop){
    schedulemod->insertRows(toTop?0:schedulemod->rowCount(),1);
    QStandardItem *item = new QStandardItem(QString::fromStdString(text));
    item->setEditable(false);
    item->setDropEnabled(false);
    schedulemod->setItem(toTop?0:(schedulemod->rowCount()-1), 0, item);
    return item;
}
void pgWrite::onScan(){
    pgMGUI->chooseObj(true);
    pgMGUI->move(scanCoords[0], scanCoords[1], scanCoords[2], true);
    pgSGUI->doNRounds(scanRepeatN->val,scanROI);
    res=scanRes->get();
    if(res==nullptr){QMessageBox::critical(gui_activation, "Error", "Somehow cannot find scan.\n");return;}
    saveB->setEnabled(true);
}
void pgWrite::onSave(){
    res=scanRes->get();
    if(res==nullptr){saveB->setEnabled(false);return;}
    std::string filename=filenaming->get();
    replacePlaceholdersInString(filename);
    std::string path=util::toString(scan_default_folder->get(),"/",filename);
    size_t found=path.find_last_of("/");
    if(found!=std::string::npos){
        path.erase(found,path.size()+found);
        cv::utils::fs::createDirectory(path);
    }
    filename=QFileDialog::getSaveFileName(gui_activation,"Save scan to file.",QString::fromStdString(util::toString(scan_default_folder->get(),"/",filename,".pfm")),"Images (*.pfm)").toStdString();
    if(filename.empty()) return;
    pgScanGUI::saveScan(res,filename);
    saveConfig(filename);
}
void pgWrite::saveConfig(std::string filename){
    if(filename.size()>4) filename.replace(filename.size()-4,4,".cfg");
    std::ofstream wfile(filename);
    wfile<<genConfig();
    wfile.close();
}
std::string pgWrite::genConfig(){
    std::string fn=fileName;
    size_t found=fn.find_last_of("/");
    if(found!=std::string::npos) fn.erase(0,found+1);
    std::string ret= util::toString(
                "Tag String: ",tagString->text().toStdString(),"\n",
                "Tag UInt: ",static_cast<int>(tagUInt->val),"\n",
                "MaxDepth: ",lastDepth,"\n",
                "ImgUmPPx: ",imgUmPPx->val,"\n",
                "Source filename: ",fn,"\n"
                );
    if(addNotes->val){
        ret+="User Notes:\n";
        ret+=notestring;
    }
    return ret;
}
void pgWrite::on_write_default_folder(){
    std::string folder=QFileDialog::getExistingDirectory(gui_settings, tr("Select write source default folder"), QString::fromStdString(write_default_folder->get())).toStdString();
    if(!folder.empty()) write_default_folder->set(folder);
}
void pgWrite::on_scan_default_folder(){
    std::string folder=QFileDialog::getExistingDirectory(gui_settings, tr("Select autoscan destination default folder"), QString::fromStdString(scan_default_folder->get())).toStdString();
    if(!folder.empty()) scan_default_folder->set(folder);
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
    wasMirau=pgMGUI->currentObj==0;
    pgMGUI->chooseObj(false);    // switch to writing
    CTRL::CO co(go.pRPTY);
    co.addHold("X",CTRL::he_motion_ontarget);
    co.addHold("Y",CTRL::he_motion_ontarget);
    co.addHold("Z",CTRL::he_motion_ontarget);
    co.pulseGPIO("wrLaser",pulseDur->val/1000);
    co.execute();
    if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true);
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
    fileName=QFileDialog::getOpenFileName(gui_activation,"Select file containing stucture to write (Should be either 8bit or 16bit image (will be made monochrome if color), or 32bit float.).",
                                          firstImageLoaded?"":QString::fromStdString(write_default_folder->get()),"Images (*.pfm *.png *.jpg *.tif)").toStdString();
    if(fileName.empty()) return;
    WRImage=cv::imread(fileName, cv::IMREAD_GRAYSCALE|cv::IMREAD_ANYDEPTH);
    if(WRImage.empty()) {
        QMessageBox::critical(gui_activation, "Error", "Image empty.\n");
        return;
    }
    if(WRImage.type()!=CV_8U && WRImage.type()!=CV_16U && WRImage.type()!=CV_32F){
        QMessageBox::critical(gui_activation, "Error", "Image type not compatible.\n");
        return;
    }

    firstImageLoaded=true;
    writeDM->setEnabled(true);
}
void pgWrite::onWriteDM(){
    // save coords for scan
    pgMGUI->wait4motionToComplete();
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    pgMGUI->getPos(&scanCoords[0], &scanCoords[1], &scanCoords[2]);

    prepareScanROI();
    if(WRImage.type()!=CV_32F) lastDepth=depthMaxval->val;
    else cv::minMaxIdx(WRImage,0,&lastDepth);

    if(!firstWritten)firstWritten=true;
    else tagUInt->setValue(tagUInt->val+1);

    if(useWriteScheduling->val){    // scheduling
        std::string filename=filenaming->get();
        replacePlaceholdersInString(filename);
        scheduled.emplace_back();
        for(int i:{0,1,2})scheduled.back().coords[i]=scanCoords[i];
        WRImage.copyTo(scheduled.back().src);
        scheduled.back().isWrite=true;
        scheduled.back().writePars[0]=depthMaxval->val/1000000;
        scheduled.back().writePars[1]=imgUmPPx->val/1000;
        scheduled.back().writePars[2]=pointSpacing->val/1000;
        scheduled.back().writePars[3]=focus->val/1000;
        scheduled.back().writePars[4]=focusXcor->val/1000;
        scheduled.back().writePars[5]=focusYcor->val/1000;
        scheduled.back().ptr=addScheduleItem(util::toString("Pending - Write       - ",filename),true);
        cv::Mat resized;
        double ratio=imgUmPPx->val;
        double xSize=round(ratio*WRImage.cols)*1000/pgMGUI->getNmPPx(0);
        double ySize=round(ratio*WRImage.rows)*1000/pgMGUI->getNmPPx(0);
        cv::resize(WRImage, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
        scheduled.back().overlay=ovl.add_overlay(resized,pgMGUI->mm2px(scanCoords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(scanCoords[1]-pgBeAn->writeBeamCenterOfsY,0));

        scheduled.emplace_back();
        for(int i:{0,1,2})scheduled.back().coords[i]=scanCoords[i];
        scheduled.back().isWrite=false;
        scheduled.back().filename=util::toString(scan_default_folder->get(),"/",filename);
        scheduled.back().conf=genConfig();
        scheduled.back().scanROI=scanROI;
        scheduled.back().ptr=addScheduleItem(util::toString("Pending - Scan        - ",filename),false);

        return;
    }

    scanB->setEnabled(false);
    saveB->setEnabled(false);
    //write:
    if(writeMat()){
        firstWritten=false;
        return;
    }
    scanB->setEnabled(true);
}
void pgWrite::prepareScanROI(){
    int xSize=pgMGUI->mm2px(round(imgUmPPx->val*WRImage.cols+2)/1000,0);
    int ySize=pgMGUI->mm2px(round(imgUmPPx->val*WRImage.rows+2)/1000,0);
    int cols=go.pGCAM->iuScope->camCols;
    int rows=go.pGCAM->iuScope->camRows;

    double extraB=scanExtraBorder->val;
    if(extraB>0){
        if(scanExtraBorder->index==0){  // um
            extraB=pgMGUI->mm2px(extraB/1000,0);
        }
    }
    scanROI=cv::Rect(cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX,0)-extraB, rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY,0)-extraB, xSize+2*extraB, ySize+2*extraB);
    if(scanROI.x<0){
        scanROI.width+=scanROI.x;
        scanROI.x=0;
    }
    if(scanROI.y<0){
        scanROI.height+=scanROI.y;
        scanROI.y=0;
    }
    if(scanROI.width+scanROI.x>cols) scanROI.width=cols-scanROI.x;
    if(scanROI.height+scanROI.y>rows) scanROI.height=rows-scanROI.y;
}
bool pgWrite::writeMat(cv::Mat* override, double override_depthMaxval, double override_imgmmPPx, double override_pointSpacing, double override_focus, double ov_fxcor, double ov_fycor){
    wabort=false;
    cv::Mat tmpWrite, resizedWrite;
    cv::Mat* src;
    double vdepthMaxval, vimgmmPPx, vpointSpacing, vfocus, vfocusXcor, vfocusYcor;
    src=(override!=nullptr)?override:&WRImage;
    vdepthMaxval=(override_depthMaxval!=0)?override_depthMaxval:depthMaxval->val/1000000;
    vimgmmPPx=(override_imgmmPPx!=0)?override_imgmmPPx:imgUmPPx->val/1000;
    vpointSpacing=(override_pointSpacing!=0)?override_pointSpacing:pointSpacing->val/1000;
    vfocus=(override_focus!=std::numeric_limits<double>::max())?override_focus:focus->val/1000;
    vfocusXcor=(ov_fxcor!=std::numeric_limits<double>::max())?ov_fxcor:focusXcor->val/1000;
    vfocusYcor=(ov_fycor!=std::numeric_limits<double>::max())?ov_fycor:focusYcor->val/1000;

    if(src->type()==CV_8U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/255);
    }else if(src->type()==CV_16U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/65536);
    }else if(src->type()==CV_32F) src->copyTo(tmpWrite);
    else {QMessageBox::critical(gui_activation, "Error", "Image type not compatible.\n"); return true;}

    double ratio=vimgmmPPx/vpointSpacing;
    if(vimgmmPPx==vpointSpacing) tmpWrite.copyTo(resizedWrite);
    else if(vimgmmPPx<vpointSpacing) cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_AREA);   //shrink
    else cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_CUBIC);                         //enlarge

    // at this point resizedWrite contains the desired depth at each point
    std::lock_guard<std::mutex>lock(MLP._lock_proc);
    if(!go.pRPTY->connected) return true;
    wasMirau=pgMGUI->currentObj==0;
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

    double pulse;
    bool xdir=0;
    abort->setVisible(true);
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
        if(wabort) {if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true); abort->setVisible(false); return true;}
    }

    pgMGUI->corCOMove(CO,-vfocusXcor,-vfocusYcor,-vfocus);
    pgMGUI->corCOMove(CO,(xdir?-1:1)*offsX/2,-offsY/2,0);
    CO.execute();
    CO.clear(true);

    while(CO.getProgress()<1) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    MLP.progress_proc=100;
    if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true);
    abort->setVisible(false);
    return false;
}
void pgWrite::onWriteTag(){
    cv::Size size=cv::getTextSize(tagText->text().toStdString(), OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, settingWdg[4]->fontThickness->val, nullptr);
    tagImage=cv::Mat(size.height+4,size.width+4,CV_8U,cv::Scalar(0));
    cv::putText(tagImage,tagText->text().toStdString(), {0,size.height+1}, OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, cv::Scalar(255), settingWdg[4]->fontThickness->val, cv::LINE_AA);

    if(useWriteScheduling->val){
        pgMGUI->wait4motionToComplete();
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
        double coords[3];
        pgMGUI->getPos(&coords[0], &coords[1], &coords[2]);

        std::string filename=filenaming->get();
        replacePlaceholdersInString(filename);
        scheduled.emplace_back();
        for(int i:{0,1,2})scheduled.back().coords[i]=coords[i];
        tagImage.copyTo(scheduled.back().src);
        scheduled.back().isWrite=true;
        scheduled.back().writePars[0]=settingWdg[4]->depthMaxval->val/1000000;
        scheduled.back().writePars[1]=settingWdg[4]->imgUmPPx->val/1000;
        scheduled.back().writePars[2]=settingWdg[4]->pointSpacing->val/1000;
        scheduled.back().writePars[3]=settingWdg[4]->focus->val/1000;
        scheduled.back().writePars[4]=settingWdg[4]->focusXcor->val/1000;
        scheduled.back().writePars[5]=settingWdg[4]->focusYcor->val/1000;
        scheduled.back().ptr=addScheduleItem(util::toString("Pending - ","Write Tag   - ",filename),true);
        cv::Mat resized;
        double ratio=settingWdg[4]->imgUmPPx->val;
        double xSize=round(ratio*tagImage.cols)*1000/pgMGUI->getNmPPx(0);
        double ySize=round(ratio*tagImage.rows)*1000/pgMGUI->getNmPPx(0);
        cv::resize(tagImage, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
        scheduled.back().overlay=ovl.add_overlay(resized,pgMGUI->mm2px(coords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(coords[1]-pgBeAn->writeBeamCenterOfsY,0));
    }
    else writeMat(&tagImage,settingWdg[4]->depthMaxval->val/1000000,settingWdg[4]->imgUmPPx->val/1000, settingWdg[4]->pointSpacing->val/1000,settingWdg[4]->focus->val/1000,settingWdg[4]->focusXcor->val/1000,settingWdg[4]->focusYcor->val/1000);
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
void pgWrite::onChangeDrawScanAreaOn(bool status){
    drawWriteAreaOn=status?3:0;
}
void pgWrite::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double ratio;
    double xSize;
    double ySize;
    double xShiftmm=0;
    double yShiftmm=0;

    if(drawWriteAreaOn==2){ //tag
        if(tagText->text().toStdString().empty()) return;
        ratio=settingWdg[4]->imgUmPPx->val;
        cv::Size size=cv::getTextSize(tagText->text().toStdString(), OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, settingWdg[4]->fontThickness->val, nullptr);
        xSize=round(ratio*(size.width+4))*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*(size.height+4))*1000/pgMGUI->getNmPPx();
    }else if(drawWriteAreaOn==1){ // write
        if(WRImage.empty() || !writeDM->isEnabled()) return;
        ratio=imgUmPPx->val;
        xSize=round(ratio*WRImage.cols)*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*WRImage.rows)*1000/pgMGUI->getNmPPx();
    }else if(drawWriteAreaOn==3){  // scan
        if(!scanB->isEnabled() || !go.pRPTY->connected) return;
        pgMGUI->getPos(&xShiftmm, &yShiftmm);
        xShiftmm-=scanCoords[0];
        yShiftmm-=scanCoords[1];
        xSize=pgMGUI->mm2px(pgMGUI->px2mm(scanROI.width,0));
        ySize=pgMGUI->mm2px(pgMGUI->px2mm(scanROI.height,0));
    }

    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX+xShiftmm), img->rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY+yShiftmm), xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
