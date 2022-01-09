#include "write.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "GUI/tab_monitor.h"    //for debug purposes
#include <time.h>
#include "opencv2/core/utils/filesystem.hpp"
#include <filesystem>
#include <QTreeView>
#include <QStandardItemModel>
#include <gsl/gsl_multifit.h>

pgWrite::pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI, procLockProg& MLP, pgScanGUI* pgSGUI, overlay& ovl, pgFocusGUI* pgFGUI): pgBeAn(pgBeAn), pgMGUI(pgMGUI), MLP(MLP), pgSGUI(pgSGUI), ovl(ovl), pgFGUI(pgFGUI){
    gui_activation=new QWidget;
    gui_settings=new QWidget;
    scanRes=pgSGUI->result.getClient();

    alayout=new QVBoxLayout;
    slayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    gui_settings->setLayout(slayout);

    pulse=new QPushButton("Pulse");
    connect(pulse, SIGNAL(released()), this, SLOT(onPulse()));
    pulseDur=new val_selector(1, "Duration:", 0.001, 10000, 3, 0, {"ms","nm"});
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
    schedulelw=new QTreeView;
    schedulemod=new QStandardItemModel(0, 3);
    schedulemod->setHorizontalHeaderLabels({"Status","Type","Name"});
    schedulelw->setModel(schedulemod);
    schedulelw->setDragDropMode(QAbstractItemView::DropOnly);
    schedulelw->setDefaultDropAction(Qt::MoveAction);
    schedulelw->setItemsExpandable(false);
    schedulelw->setDragDropOverwriteMode(false);
    schedulelw->setRootIsDecorated(false);

    itemMoveTop=new QPushButton();
    itemMoveTop->setIcon(QPixmap(":/top.svg"));
    connect(itemMoveTop, SIGNAL(released()), this, SLOT(onItemMoveTop()));
    itemMoveTop->setToolTip("Move selected item to top.");
    itemMoveUp=new QPushButton();
    itemMoveUp->setIcon(QPixmap(":/up.svg"));
    connect(itemMoveUp, SIGNAL(released()), this, SLOT(onItemMoveUp()));
    itemMoveUp->setToolTip("Move selected item one row up.");
    itemMoveDown=new QPushButton();
    itemMoveDown->setIcon(QPixmap(":/down.svg"));
    connect(itemMoveDown, SIGNAL(released()), this, SLOT(onItemMoveDown()));
    itemMoveDown->setToolTip("Move selected item one row down.");
    itemMoveBottom=new QPushButton();
    itemMoveBottom->setIcon(QPixmap(":/bottom.svg"));
    connect(itemMoveBottom, SIGNAL(released()), this, SLOT(onItemMoveBottom()));
    itemMoveBottom->setToolTip("Move selected item to bottom.");
    itemRemove=new QPushButton();
    itemRemove->setIcon(QPixmap(":/stock_no.svg"));
    connect(itemRemove, SIGNAL(released()), this, SLOT(onItemRemove()));
    itemRemove->setToolTip("Remove selected item.");
    clearNonPending=new QPushButton();
    clearNonPending->setIcon(QPixmap(":/edit-clear.svg"));
    connect(clearNonPending, SIGNAL(released()), this, SLOT(onClearNonPending()));
    clearNonPending->setToolTip("Remove all non-pending items.");
    schedulelwtwid=new twid(schedulelw,new vtwid(itemMoveTop,itemMoveUp,itemMoveDown,itemMoveBottom,itemRemove,clearNonPending,false,false),false,false);
    schedulelwtwid->setVisible(false);
    alayout->addWidget(schedulelwtwid);

    scheduleWriteStart=new QPushButton("Execute list");
    scheduleWriteStart->setVisible(false);
    scheduleWriteStart->setEnabled(false);
    connect(scheduleWriteStart, SIGNAL(released()), this, SLOT(onScheduleWriteStart()));
    alayout->addWidget(new twid(scheduleWriteStart));

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
    QLabel* textl=new QLabel("The required pulse duration T to write a peak of height H is determined via T=A*H+C. If bsplines are defined, they overwrite this (only within their defined range).");
    textl->setWordWrap(true);
    slayout->addWidget(textl);
    slayout->addWidget(new hline());
    refocusBeforeWrite=new checkbox_gs(false,"Refocus before each write");
    conf["refocusBeforeWrite"]=refocusBeforeWrite;
    slayout->addWidget(refocusBeforeWrite);
    writeZeros=new checkbox_gs(false,"Write zeros");
    writeZeros->setToolTip("If enabled, areas that do not need extra height will be written at threshold duration anyway (slower writing but can give more consistent zero levels if calibration is a bit off).");
    conf["writeZeros"]=writeZeros;
    slayout->addWidget(writeZeros);
    selPScheduling=new smp_selector("Point scheduling: ", 0, {"ZigZag","Nearest"});
    conf["selPScheduling"]=selPScheduling;
    slayout->addWidget(selPScheduling);

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

    p_coefs=new gsl_vector;
    p_covmat=new gsl_matrix;
    p_gbreakpts=new gsl_vector;
}
pgWrite::~pgWrite(){
    delete scanRes;
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
    for(int i:{3,4}) if(replacements[i].back()=='.') replacements[i].pop_back();    //if integer remove point
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
    std::vector<std::filesystem::path> folders;

    folders.push_back(scan_default_folder->get());
    int newTagN=-1;
    while(!folders.empty()){
        std::vector<std::filesystem::directory_entry> files;
        for(auto const& dir_entry: std::filesystem::directory_iterator{folders.back()}) files.push_back(dir_entry);
        folders.pop_back();
        for(auto& path: files){
            if(path.is_directory()) folders.push_back(path);
            else if(path.path().extension().string()==".cfg"){
                std::ifstream ifile(path.path());
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
    if(newTagN>-1) tagUInt->setValue(newTagN+1);
}
void pgWrite::onUseWriteScheduling(bool state){
    writeDM->setText(state?"Schedule Write":"Write");
    writeTag->setText(state?"Schedule Write Tag":"Write Tag");
    schedulelwtwid->setVisible(state);
    saveB->setVisible(!state);
    scanB->setVisible(!state);
    scheduleWriteStart->setVisible(state);
    scheduleWriteStart->setEnabled(pendingInScheduleList());
    ovl.enabled=state;
}
QStandardItem* pgWrite::addScheduleItem(std::string status, std::string type, std::string name, bool toTop){
    schedulemod->insertRows(toTop?0:schedulemod->rowCount(),1);
    int i=0;
    for(auto text:{status,type,name}){
        QStandardItem *item = new QStandardItem(QString::fromStdString(text));
        item->setEditable(false);
        schedulemod->setItem(toTop?0:(schedulemod->rowCount()-1), i, item);
        schedulelw->resizeColumnToContents(i);
        i++;
    }
    scheduleWriteStart->setEnabled(pendingInScheduleList());
    return schedulemod->item(toTop?0:(schedulemod->rowCount()-1), 0);
}
void pgWrite::onItemMoveTop(){
    auto ci=schedulelw->currentIndex();
    if(!ci.isValid()) return;
    auto orow=schedulemod->takeRow(ci.row());
    schedulemod->insertRow(0,orow);
    schedulelw->setCurrentIndex(schedulemod->index(0,0));
}
void pgWrite::onItemMoveUp(){
    auto ci=schedulelw->currentIndex();
    if(!ci.isValid() || ci.row()<=0) return;
    auto orow=schedulemod->takeRow(ci.row());
    schedulemod->insertRow(ci.row()-1,orow);
    schedulelw->setCurrentIndex(schedulemod->index(ci.row()-1,0));
}
void pgWrite::onItemMoveDown(){
    auto ci=schedulelw->currentIndex();
    if(!ci.isValid() || ci.row()>=schedulemod->rowCount()-1) return;
    auto orow=schedulemod->takeRow(ci.row());
    schedulemod->insertRow(ci.row()+1,orow);
    schedulelw->setCurrentIndex(schedulemod->index(ci.row()+1,0));
}
void pgWrite::onItemMoveBottom(){
    auto ci=schedulelw->currentIndex();
    if(!ci.isValid()) return;
    auto orow=schedulemod->takeRow(ci.row());
    schedulemod->insertRow(schedulemod->rowCount(),orow);
    schedulelw->setCurrentIndex(schedulemod->index(schedulemod->rowCount()-1,0));
}
void pgWrite::onItemRemove(){
    auto ci=schedulelw->currentIndex();
    if(!ci.isValid()) return;
    for(std::vector<schItem>::iterator it=scheduled.begin();it!=scheduled.end();++it){
        if(it->ptr==schedulemod->item(ci.row(),0)){
            if(it->overlay!=nullptr) ovl.rm_overlay(it->overlay);
            scheduled.erase(it);
            break;
        }
    }
    schedulemod->removeRow(ci.row());
    scheduleWriteStart->setEnabled(pendingInScheduleList());
}
void pgWrite::onClearNonPending(){
    for(int i=0;i!=schedulemod->rowCount();i++){
        if(schedulemod->item(i,0)->text()!="Pending"){
            schedulemod->removeRow(i);
            i--;
        }
    }
}
unsigned pgWrite::pendingInScheduleList(){
    unsigned ret=0;
    for(std::vector<schItem>::iterator it=scheduled.begin();it!=scheduled.end();++it){
        if(it->pending) ret++;
    }
    return ret;
}
void pgWrite::onScan(){
    if(_onScan()) QMessageBox::critical(gui_activation, "Error", "Scan failed.\n");
    else saveB->setEnabled(true);
}
bool pgWrite::_onScan(cv::Rect ROI, double* coords){
    pgMGUI->chooseObj(true);
    if(coords!=nullptr) pgMGUI->move(coords[0], coords[1], coords[2], true);
    else pgMGUI->move(scanCoords[0], scanCoords[1], scanCoords[2], true);
    if(pgSGUI->doNRounds(scanRepeatN->val,ROI.width==0?scanROI:ROI,maxRedoScanTries)) return true;
    res=scanRes->get();
    if(res==nullptr){std::cerr<<"Somehow cannot find scan in pgWrite::_onScan()\n";return true;}
    return false;
}
void pgWrite::onSave(){
    _onSave(true);
}
bool pgWrite::_onSave(bool ask, std::string filename){
    res=scanRes->get();
    if(res==nullptr){saveB->setEnabled(false);return true;}
    if(filename=="") filename=filenaming->get();
    replacePlaceholdersInString(filename);
    std::string path=util::toString(scan_default_folder->get(),"/",filename);
    size_t found=path.find_last_of("/");
    if(found!=std::string::npos){
        path.erase(found,path.size()+found);
        cv::utils::fs::createDirectory(path);
    }
    if(ask){
        filename=QFileDialog::getSaveFileName(gui_activation,"Save scan to file.",QString::fromStdString(util::toString(scan_default_folder->get(),"/",filename,".pfm")),"Images (*.pfm)").toStdString();
        if(filename.empty()) return true;
    }
    pgScanGUI::saveScan(res,filename);
    saveConfig(filename);
    return false;
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
void pgWrite::onCorPPR(){
    bool ok;
    float preH=QInputDialog::getDouble(gui_activation, "Correct plateau-peak ratio", "Input actual written plateau height (nm) for given expected peak height.", 0.001, 0, 1000, 3, &ok);
    if(!ok) return;
    float cDT=predictDuration(depthMaxval->val);
    float gDT=predictDuration(preH);
    plataeuPeakRatio->setValue(plataeuPeakRatio->val*gDT/cDT);
}
void pgWrite::onPulse(){
    if(!go.pRPTY->connected) return;
    wasMirau=pgMGUI->currentObj==0;
    pgMGUI->chooseObj(false);    // switch to writing
    double pulse, vfocus{0}, vfocusXcor{0}, vfocusYcor{0};
    if(pulseDur->index==0) pulse=pulseDur->val;
    else{
        preparePredictor();
        pulse=predictDuration(pulseDur->val);
        vfocus=focus->val/1000;
        vfocusXcor=focusXcor->val/1000;
        vfocusYcor=focusYcor->val/1000;
    }
    CTRL::CO co(go.pRPTY);
    pgMGUI->corCOMove(co,vfocusXcor,vfocusYcor,vfocus);
    co.addHold("X",CTRL::he_motion_ontarget);
    co.addHold("Y",CTRL::he_motion_ontarget);
    co.addHold("Z",CTRL::he_motion_ontarget);
    co.pulseGPIO("wrLaser",pulse/1000);
    pgMGUI->corCOMove(co,-vfocusXcor,-vfocusYcor,-vfocus);
    co.execute();
    if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true);
}
void pgWrite::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    focus=settingWdg[index]->focus;
    focusXcor=settingWdg[index]->focusXcor;
    focusYcor=settingWdg[index]->focusYcor;
    usingBSpline=settingWdg[index]->usingBSpline;
    constA=settingWdg[index]->constA;
    constC=settingWdg[index]->constC;
    plataeuPeakRatio=settingWdg[index]->plataeuPeakRatio;
    pointSpacing=settingWdg[index]->pointSpacing;
    bsplbreakpts=&settingWdg[index]->bsplbreakpts;
    bsplcoefs=&settingWdg[index]->bsplcoefs;
    bsplcov=&settingWdg[index]->bsplcov;
    p_ready=false;
}
writeSettings::writeSettings(uint num, pgWrite* parent): parent(parent), p_ready(parent->p_ready){
    name=parent->selectWriteSetting->getLabel(num);
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    focus=new val_selector(0, "Focus Z offset", -1000, 1000, 3, 0, {"um"});
    parent->conf[name]["focus"]=focus;
    slayout->addWidget(focus);
    focusXcor=new val_selector(0, "Focus X offset", -1000, 1000, 3, 0, {"um"});
    parent->conf[name]["focusXcor"]=focusXcor;
    slayout->addWidget(focusXcor);
    focusYcor=new val_selector(0, "Focus Y offset", -1000, 1000, 3, 0, {"um"});
    parent->conf[name]["focusYcor"]=focusYcor;
    slayout->addWidget(focusYcor);
    usingBSpline=new checkbox_gs(false,"Using bspline (uncheck to clear coefs.)");
    connect(usingBSpline, SIGNAL(changed(bool)), this, SLOT(onUsingBSpline(bool)));
    usingBSpline->setEnabled(false);
    parent->conf[name]["usingBSpline"]=usingBSpline;
    slayout->addWidget(usingBSpline);
    parent->conf[name]["bsplbreakpts"]=bsplbreakpts;
    parent->conf[name]["bsplcoefs"]=bsplcoefs;
    parent->conf[name]["bsplcov"]=bsplcov;
    constA=new val_selector(0.01, "Constant A", 0, 1000, 9, 0, {"ms/nm"});
    parent->conf[name]["constA"]=constA;
    slayout->addWidget(constA);
    constC=new val_selector(0, "Constant C", -1000, 1000, 9, 0, {"ms"});
    parent->conf[name]["constC"]=constC;
    slayout->addWidget(constC);
    plataeuPeakRatio=new val_selector(1, "Plataeu-Peak height ratio", 1, 10, 3);
    parent->conf[name]["plataeuPeakRatio"]=plataeuPeakRatio;
    corPPR=new QPushButton("Correct PPR");
    slayout->addWidget(new twid(plataeuPeakRatio,corPPR));
    pointSpacing=new val_selector(1, "Point spacing", 0.01, 10, 3, 0, {"um"});
    parent->conf[name]["pointSpacing"]=pointSpacing;
    slayout->addWidget(pointSpacing);
    if(num==4){ //tag settings
        fontFace=new smp_selector("Font ", 0, OCV_FF::qslabels());
        parent->conf[name]["fontFace"]=fontFace;
        slayout->addWidget(fontFace);
        fontSize=new val_selector(1., "Font Size ", 0, 10., 2);
        parent->conf[name]["fontSize"]=fontSize;
        slayout->addWidget(fontSize);
        fontThickness=new val_selector(1, "Font Thickness ", 1, 10, 0, 0, {"px"});
        parent->conf[name]["fontThickness"]=fontThickness;
        slayout->addWidget(fontThickness);
        imgUmPPx=new val_selector(10, "Scaling: ", 0.001, 100, 3, 0, {"um/Px"});
        parent->conf[name]["imgUmPPx"]=imgUmPPx;
        slayout->addWidget(imgUmPPx);
        depthMaxval=new val_selector(10, "Maxval=", 0.1, 500, 3, 0, {"nm"});
        parent->conf[name]["depthMaxval"]=depthMaxval;
        slayout->addWidget(depthMaxval);
        frameDis=new val_selector(10, "Frame Distance=", 0.1, 500, 3, 0, {"um"});
        parent->conf[name]["frameDis"]=frameDis;
        slayout->addWidget(frameDis);
    }
}
void writeSettings::onUsingBSpline(bool state){
    usingBSpline->setEnabled(state);
    if(!state){
        bsplbreakpts.clear();
        bsplcoefs.clear();
        constA->setValue(100);
        constC->setValue(0);
        p_ready=false;
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
    if(!useWriteScheduling->val && writeDM->text()=="Abort"){
         wabort=true;
         writeDM->setEnabled(false);
         return;
    }

    // save coords for scan
    while(go.pRPTY->getMotionSetting("",CTRL::mst_position_update_pending)!=0) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
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
        scheduled.back().writePars[0]=depthMaxval->val;
        scheduled.back().writePars[1]=imgUmPPx->val/1000;
        scheduled.back().writePars[2]=pointSpacing->val/1000;
        scheduled.back().writePars[3]=focus->val/1000;
        scheduled.back().writePars[4]=focusXcor->val/1000;
        scheduled.back().writePars[5]=focusYcor->val/1000;
        scheduled.back().ptr=addScheduleItem("Pending","Write",filename,true);
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
        scheduled.back().ptr=addScheduleItem("Pending","Scan",filename,false);

        return;
    }

    scanB->setEnabled(false);
    saveB->setEnabled(false);
    writeDM->setText("Abort");
    onChangeDrawWriteAreaOn(false);
    //write:
    if(refocusBeforeWrite->val) pgFGUI->doRefocus(true, scanROI, maxRedoRefocusTries);
    if(writeMat()) firstWritten=false;

    if(firstWritten){
        scanB->setEnabled(true);
        writeDM->setEnabled(true);
    }
    writeDM->setText("Write");
    writeDM->setEnabled(true);
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
    vdepthMaxval=(override_depthMaxval!=0)?override_depthMaxval:(depthMaxval->val.load());
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
    preparePredictor();

    double ratio=vimgmmPPx/vpointSpacing;
    if(vimgmmPPx==vpointSpacing) tmpWrite.copyTo(resizedWrite);
    else if(vimgmmPPx<vpointSpacing) cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_AREA);   //shrink
    else cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_CUBIC);                         //enlarge

    // at this point resizedWrite contains the desired depth at each point
    std::lock_guard<std::mutex>lock(MLP._lock_proc);
    if(!go.pRPTY->connected) return true;
    wasMirau=pgMGUI->currentObj==0;
    pgMGUI->chooseObj(false);    // switch to writing

    CTRL::CO CO(go.pRPTY);
    double offsX,offsY;
    offsX=(resizedWrite.cols-1)*vpointSpacing;
    offsY=(resizedWrite.rows-1)*vpointSpacing;
        // move to target and correct for focus (this can happen simultaneously with objective switch)
    pgMGUI->corCOMove(CO,vfocusXcor,vfocusYcor,vfocus);
    pgMGUI->corCOMove(CO,-offsX/2,-offsY/2,0);
    if(selPScheduling->index==0){   //ZigZag

            // wait till motion completes
        CO.addHold("X",CTRL::he_motion_ontarget);
        CO.addHold("Y",CTRL::he_motion_ontarget);
        CO.addHold("Z",CTRL::he_motion_ontarget);
        CO.execute();
        CO.clear(true);

        double pulse;
        bool xdir=0;
        for(int j=0;j!=resizedWrite.rows;j++){   // write row by row (so that processing for the next row is done while writing the previous one, and the operation can be terminated more easily)
            for(int i=0;i!=resizedWrite.cols;i++){
                pulse=predictDuration(resizedWrite.at<float>(resizedWrite.rows-j-1,xdir?(resizedWrite.cols-i-1):i));        // Y inverted because image formats have flipped Y
                if(i!=0) pgMGUI->corCOMove(CO,(xdir?-1:1)*vpointSpacing,0,0);
                if(writeZeros->val==false && resizedWrite.at<float>(resizedWrite.rows-j-1,xdir?(resizedWrite.cols-i-1):i)==0) continue;
                if(pulse==0) continue;
                CO.addHold("X",CTRL::he_motion_ontarget);
                CO.addHold("Y",CTRL::he_motion_ontarget);
                CO.addHold("Z",CTRL::he_motion_ontarget);   // because corCOMove may correct Z too
                CO.pulseGPIO("wrLaser",pulse/1000);
            }
            if (j!=resizedWrite.rows-1) pgMGUI->corCOMove(CO,0,vpointSpacing,0);
            CO.execute();
            CO.clear(true);
            xdir^=1;

            while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
            MLP.progress_proc=100./resizedWrite.rows*j;
            if(wabort) {pgMGUI->move(0,0,-vfocus); if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true); return true;}
        }

        pgMGUI->corCOMove(CO,-vfocusXcor,-vfocusYcor,-vfocus);
        pgMGUI->corCOMove(CO,(xdir?-1:1)*offsX/2,-offsY/2,0);
        CO.execute();
        CO.clear(true);
    }else if(selPScheduling->index==1){ //Nearest
        cv::flip(resizedWrite,resizedWrite,0);
        cv::Mat completed=cv::Mat::zeros(resizedWrite.size(), CV_8U);
        if(writeZeros->val==false) cv::compare(resizedWrite,0,completed,cv::CMP_EQ);
        cv::Mat pulseMat=cv::Mat::zeros(resizedWrite.size(), CV_32F);
        for(int j=0;j!=resizedWrite.rows;j++) for(int i=0;i!=resizedWrite.cols;i++) pulseMat.at<float>(j,i)=predictDuration(resizedWrite.at<float>(j,i));
        int last_i=0, last_j=0;
        int next_i=last_i, next_j=last_j;

        long todo=completed.rows*completed.cols-cv::countNonZero(completed);
        const long total=completed.rows*completed.cols;
        if(todo==0) goto abort;

        struct pt{
            int i,j;
            double distance;    // in um
        };
        std::vector<pt> lut;
        const double velX=go.pRPTY->getMotionSetting("X",CTRL::mst_defaultVelocity);
        const double velY=go.pRPTY->getMotionSetting("Y",CTRL::mst_defaultVelocity);
        const double accX=go.pRPTY->getMotionSetting("X",CTRL::mst_defaultAcceleration);
        const double accY=go.pRPTY->getMotionSetting("Y",CTRL::mst_defaultAcceleration);
        double timeX,timeY;

        for(int j=0;j!=resizedWrite.rows;j++) for(int i=0;i!=resizedWrite.cols;i++){
            mcutil::evalAccMotion(i*vpointSpacing, accX, velX, &timeX);
            mcutil::evalAccMotion(j*vpointSpacing, accY, velY, &timeY);
            lut.push_back({i,j,sqrt(pow(i*vpointSpacing,2)+pow(j*vpointSpacing,2))});
        }
        std::sort(lut.begin(), lut.end(), [](pt i,pt j){return (i.distance<j.distance);});

        if(completed.at<uint8_t>(next_j,next_i)==255) for(auto& el: lut)
            if(completed.at<uint8_t>(el.j,el.i)==0){
                next_j=el.j;
                next_i=el.i;
                goto _out;
            }_out:;

        int nPerRun=sqrt(resizedWrite.rows*resizedWrite.cols);
        if(nPerRun<100) nPerRun=100;
        while(1){
            for(int i=0;i!=nPerRun; i++){
                pgMGUI->corCOMove(CO,(next_i-last_i)*vpointSpacing,(next_j-last_j)*vpointSpacing,0);
                last_i=next_i;
                last_j=next_j;
                CO.addHold("X",CTRL::he_motion_ontarget);
                CO.addHold("Y",CTRL::he_motion_ontarget);
                CO.addHold("Z",CTRL::he_motion_ontarget);   // because corCOMove may correct Z too
                CO.pulseGPIO("wrLaser",predictDuration(resizedWrite.at<float>(next_j,next_i))/1000);
                completed.at<uint8_t>(next_j,next_i)=255;
                todo--;

                if(todo==0) break;
                for(auto& el: lut) for(int tmp_i:{last_i-el.i,last_i+el.i}) for(int tmp_j:{last_j-el.j,last_j+el.j}){     // there is some redundancy
                    if(tmp_i<0 || tmp_i>=completed.cols || tmp_j<0 || tmp_j>=completed.rows) continue;
                    if(completed.at<uint8_t>(tmp_j,tmp_i)==0){
                        next_i=tmp_i;
                        next_j=tmp_j;
                        goto next;
                    }
                }
                QMessageBox::critical(gui_activation, "Error", "Cannot find a point in pgWrite::writeMat; this shouldn't happen!"); goto abort; // in case there is an unforseen bug, TODO remove
                next:;
            }
            CO.execute();
            CO.clear(true);
            while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
            MLP.progress_proc=100./total*(total-todo);
            if(todo==0) break;
        }

        pgMGUI->corCOMove(CO,-vfocusXcor,-vfocusYcor,-vfocus);
        pgMGUI->corCOMove(CO,-last_i*vpointSpacing+offsX/2,-last_j*vpointSpacing+offsY/2,0);
        CO.execute();
        CO.clear(true);
    }

    abort: MLP.progress_proc=100;
    if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true);
    return false;
}
void pgWrite::onWriteTag(){
    if(!useWriteScheduling->val && writeTag->text()=="Abort"){
         wabort=true;
         writeTag->setEnabled(false);
         return;
    }

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
        scheduled.back().ptr=addScheduleItem("Pending","Write",util::toString("Tag: ",tagText->text().toStdString()),true);
        cv::Mat resized;
        double ratio=settingWdg[4]->imgUmPPx->val;
        double xSize=round(ratio*tagImage.cols)*1000/pgMGUI->getNmPPx(0);
        double ySize=round(ratio*tagImage.rows)*1000/pgMGUI->getNmPPx(0);
        cv::resize(tagImage, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
        scheduled.back().overlay=ovl.add_overlay(resized,pgMGUI->mm2px(coords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(coords[1]-pgBeAn->writeBeamCenterOfsY,0));
    }
    else{
        writeTag->setText("Abort");
        onChangeDrawWriteAreaOnTag(false);
        writeMat(&tagImage,settingWdg[4]->depthMaxval->val,settingWdg[4]->imgUmPPx->val/1000, settingWdg[4]->pointSpacing->val/1000,settingWdg[4]->focus->val/1000,settingWdg[4]->focusXcor->val/1000,settingWdg[4]->focusYcor->val/1000);
        writeTag->setText("Write Tag");
        writeTag->setEnabled(true);
    }
}

void pgWrite::preparePredictor(){
    if(p_basisfun!=nullptr) {gsl_vector_free(p_basisfun);p_basisfun=nullptr;}
    if(p_bsplws!=nullptr) {gsl_bspline_free(p_bsplws);p_bsplws=nullptr;}

    size_t ncoeffs=bsplcoefs->size();
    if(ncoeffs>4){
        size_t nbreak=ncoeffs-2;
        *p_coefs=gsl_vector_view_array(bsplcoefs->data(),ncoeffs).vector;
        *p_covmat=gsl_matrix_view_array(bsplcov->data(), ncoeffs, ncoeffs).matrix;
        *p_gbreakpts=gsl_vector_view_array(bsplbreakpts->data(),nbreak).vector;
        p_basisfun=gsl_vector_alloc(ncoeffs);
        p_bsplws=gsl_bspline_alloc(4, nbreak); // cubic bspline
        gsl_bspline_knots(p_gbreakpts,p_bsplws);
    }else p_coefs=nullptr;
    p_ready=true;
}
double pgWrite::predictDuration(double targetHeight){
    if(!p_ready) preparePredictor();
    double T{0},Terr;
    if(targetHeight<0) return T;
    if(p_coefs!=nullptr){
        if(targetHeight>gsl_vector_get(p_gbreakpts,p_gbreakpts->size-1)) goto lin;
        gsl_bspline_eval(targetHeight, p_basisfun, p_bsplws);
        gsl_multifit_linear_est(p_basisfun, p_coefs, p_covmat, &T, &Terr);
    }else lin: T=constA->val*targetHeight+constC->val;
    return T/plataeuPeakRatio->val;
}


void pgWrite::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status?(writeDM->text()=="Abort"?0:1):0;
}
void pgWrite::onChangeDrawWriteAreaOnTag(bool status){
    drawWriteAreaOn=status?(writeTag->text()=="Abort"?0:2):0;
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
void pgWrite::onScheduleWriteStart(){
    if(scheduleWriteStart->text()=="Abort current"){
        wabort=true;
        scheduleWriteStart->setEnabled(false);
        return;
    }
    if(scheduleWriteStart->text()=="Pause execution"){
        scheduleWriteStart->setText("Abort current");
        return;
    }
    scheduleWriteStart->setText("Pause execution");

    for(int i=0;i!=schedulemod->rowCount();i++){
        if(schedulemod->item(i,0)->text()!="Pending"){
            schedulemod->removeRow(i);
            i--;
        }
    }
    for(int i=0;i!=schedulemod->rowCount();i++){
        if(schedulemod->item(i,0)->text()=="Pending"){
            std::vector<schItem>::iterator it;
            for(it=scheduled.begin();it!=scheduled.end();++it)
                if(it->ptr==schedulemod->item(i,0)) break;

            schedulemod->item(i,0)->setText("Running");
            schedulelw->resizeColumnToContents(0);

            bool failed;
            if(it->isWrite){
                pgMGUI->move(it->coords[0], it->coords[1], it->coords[2], true);
                if(refocusBeforeWrite->val) pgFGUI->doRefocus(true, scanROI, maxRedoRefocusTries);
                failed=writeMat(&it->src, it->writePars[0], it->writePars[1], it->writePars[2], it->writePars[3], it->writePars[4], it->writePars[5]);
            }else{
                saveB->setEnabled(false);
                failed=_onScan(it->scanROI, it->coords);
                if(!failed){
                    if(_onSave(false, it->filename)){
                        failed=true;
                        break;
                    }
                }
            }

            ovl.rm_overlay(it->overlay);
            scheduled.erase(it);
            if(failed){
                schedulemod->item(i,0)->setText("Failed");
                break;
            }else{
                schedulemod->item(i,0)->setText("Success");
                schedulelw->resizeColumnToContents(0);
                if(scheduleWriteStart->text()=="Abort current") break;
            }
        }
    }

    scheduleWriteStart->setText("Execute list");
    scheduleWriteStart->setEnabled(pendingInScheduleList());
}
