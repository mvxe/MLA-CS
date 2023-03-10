#include "write.h"
#include "GUI/gui_includes.h"
#include "includes.h"
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
    depthScale=new val_selector(10, "(if pfm) HeightMultiplier = ", 1.0, 500, 3);
    conf["depthScale"]=depthScale;
    imgUmPPx=new val_selector(10, "Scaling: ", 0.001, 100, 3, 0, {"um/Px"});
    conf["imgUmPPx"]=imgUmPPx;
    gradualWEn=new checkbox_gs(false,"Gradual");
    conf["gradualWEn"]=gradualWEn;
    gradualW=new val_selector(10, "Step: ", 0.001, 100, 3, 0, {"nm"});
    conf["gradualW"]=gradualW;
    gradualWCut=new checkbox_gs(true,"Cut");
    conf["gradualWCut"]=gradualWCut;
    importh=new hidCon(importImg);
    importh->addWidget(depthMaxval);
    importh->addWidget(depthScale);
    importh->addWidget(imgUmPPx);
    importh->addWidget(new twid(gradualWEn,gradualW,gradualWCut));
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
    tagText->setText("tag");
    writeTag=new HQPushButton("Write Tag");
    connect(writeTag, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOnTag(bool)));
    connect(writeTag, SIGNAL(released()), this, SLOT(onWriteTag()));
    alayout->addWidget(new twid(writeTag,new QLabel("Text:"),tagText));
    rotation=new val_selector(0, "Rotation", -99999, 99999, 3, 0, {"deg"});
    alayout->addWidget(new twid(rotation));
    tagAutoUpdate= new checkbox_gs(true,"Automatic tag text");
    conf["tagAutoUpdate"]=tagAutoUpdate;
    connect(tagAutoUpdate, SIGNAL(changed(bool)), this, SLOT(onTagAutoUpdate(bool)));
    alayout->addWidget(tagAutoUpdate);
    tagString=new lineedit_gs("A");
    conf["tagString"]=tagString;
    tagSTwid=new twid(new QLabel("Tag String:"),tagString);
    tagSTwid->setVisible(false);
    connect(tagString, SIGNAL(changed()), this, SLOT(onRecomputeTagString()));
    tagUInt=new val_selector(0, "Tag UInt: ", 0, 999999, 0);
    conf["tagUInt"]=tagUInt;
    tagUInt->setVisible(false);
    connect(tagUInt, SIGNAL(changed()), this, SLOT(onRecomputeTagUInt()));
    guessTagUInt=new QPushButton("Guess");
    connect(guessTagUInt, SIGNAL(released()), this, SLOT(guessAndUpdateNextTagUInt()));
    guessTagUInt->setFlat(true);
    tagtwid=new vtwid(tagSTwid,new twid(tagUInt,guessTagUInt));
    alayout->addWidget(tagtwid);

    notes=new QPushButton("Edit notes");
    conf["notes"]=notestring;
    connect(notes, SIGNAL(released()), this, SLOT(onNotes()));
    addNotes=new checkbox_gs(false,"Append notes to .cfg");
    alayout->addWidget(new twid(notes,addNotes));

    useWriteScheduling=new checkbox_gs(false,"Enable write scheduling");
    connect(useWriteScheduling, SIGNAL(changed(bool)), this, SLOT(onUseWriteScheduling(bool)));
    scheduleScans=new checkbox_gs(true,"Schedule scans");
    conf["scheduleScans"]=scheduleScans;
    scheduleScans->setVisible(false);
    alayout->addWidget(new twid(useWriteScheduling,scheduleScans));
    schedulelw=new leQTreeView;
    schedulelw->parent=this;
    schedulelw->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    schedulemod=new QStandardItemModel(0, 3);
    schedulemod->setHorizontalHeaderLabels({"Status","Type","Name"});
    schedulelw->setModel(schedulemod);
    schedulelw->setDragDropMode(QAbstractItemView::DropOnly);
    schedulelw->setDefaultDropAction(Qt::MoveAction);
    schedulelw->setItemsExpandable(false);
    schedulelw->setDragDropOverwriteMode(false);
    schedulelw->setRootIsDecorated(false);
    schedulelw->setMouseTracking(true);
    connect(schedulelw, SIGNAL(entered(const QModelIndex&)), this, SLOT(onHoverOverScheduledItem(const QModelIndex)));

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

    itemMoveXCoord=new eadScrlBar("Move X: ", 100,20);
    connect(itemMoveXCoord->abar, SIGNAL(change(double)), this, SLOT(onItemMoveXCoord(double)));
    itemMoveXCoord->setVisible(false);
    itemMoveYCoord=new eadScrlBar("Move Y: ", 100,20);
    connect(itemMoveYCoord->abar, SIGNAL(change(double)), this, SLOT(onItemMoveYCoord(double)));
    itemMoveYCoord->setVisible(false);
    alayout->addWidget(new twid(itemMoveXCoord,itemMoveYCoord));

    scheduleWriteStart=new QPushButton("Execute list");
    scheduleWriteStart->setVisible(false);
    scheduleWriteStart->setEnabled(false);
    connect(scheduleWriteStart, SIGNAL(released()), this, SLOT(onScheduleWriteStart()));
    alayout->addWidget(new twid(scheduleWriteStart));




    writeDM->setEnabled(false);

    selectWriteSetting=new smp_selector("Select write setting: ", 0, {"Set0", "Set1", "Set2", "Set3", "Tag", "test"});    //should have Nset strings
    conf["selectWriteSetting"]=selectWriteSetting;
    slayout->addWidget(selectWriteSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new writeSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    connect(selectWriteSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    onMenuChange(0);
    WQLabel* textl=new WQLabel("The required pulse duration T to write a peak of height H is determined via T=A*H+C. If bsplines are defined, they overwrite this (only within their defined range).");
    slayout->addWidget(textl);
    slayout->addWidget(new hline());
    refocusBeforeWrite=new checkbox_gs(false,"Refocus before each write");
    conf["refocusBeforeWrite"]=refocusBeforeWrite;
    slayout->addWidget(refocusBeforeWrite);
    refocusBeforeScan=new checkbox_gs(false,"Refocus before each scan");
    conf["refocusBeforeScan"]=refocusBeforeScan;
    slayout->addWidget(refocusBeforeScan);
    writeZeros=new checkbox_gs(false,"Write zeros");
    writeZeros->setToolTip("If enabled, areas that do not need extra height will be written at threshold duration anyway (slower writing but can give more consistent zero levels if calibration is a bit off).");
    conf["writeZeros"]=writeZeros;
    slayout->addWidget(new twid(writeZeros));
    selPScheduling=new smp_selector("Point scheduling: ", 0, {"ZigZag","Nearest","Spiral"});
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
        connect(filenaming, SIGNAL(changed()), this, SLOT(onCheckTagString()));
        tagnaming=new lineedit_gs("$T$N");
        conf["tagnaming"]=tagnaming;
        folderhcon->addWidget(new twid(new QLabel("Tag autofill:"),tagnaming));
        connect(tagnaming, SIGNAL(changed()), this, SLOT(onCheckTagString()));
        std::string tooltip{"Date&Time is in ctime/strftime format (%Y, %m, %d, etc).\n"
                            "Other than that custom symbols are:\n"
                            "$T - string tag\n"
                            "$N - numeric tag\n"
                            "$F - source filename\n"
                            "$V - maxval\n"
                            "$X - scaling"};
        tagnaming->setToolTip(QString::fromStdString(tooltip));
        tooltip+="\n$W - tag text";
        filenaming->setToolTip(QString::fromStdString(tooltip));
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
    std::string placeholders[6]={"$T","$N","$F","$V","$X","$W"};
    std::string replacements[6]={tagString->text().toStdString(),std::to_string(static_cast<int>(tagUInt->val)),fileName,std::to_string(lastDepth),std::to_string(imgUmPPx->val),tagText->text().toStdString()};
    for(int i:{3,4}) if(replacements[i].find(".")!=std::string::npos) while(replacements[i].back()=='0') replacements[i].pop_back();    //remove extra zeroes
    for(int i:{3,4}) if(replacements[i].back()=='.') replacements[i].pop_back();    //if integer remove point
    found=replacements[2].find_last_of("/");
    if(found!=std::string::npos) replacements[2].erase(0,found+1);
    found=replacements[2].find_last_of(".");
    if(found!=std::string::npos) replacements[2].erase(found,replacements[2].size()-found);
    while(replacements[1].size()<numbericTagMinDigits->val) replacements[1].insert(0,"0");
    for(int i=0;i!=6;i++) while(1){
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
    if(!tagAutoUpdate->val) return;
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
    itemMoveXCoord->setVisible(state);
    itemMoveYCoord->setVisible(state);
    scheduleScans->setVisible(state);
}
void pgWrite::onTagAutoUpdate(bool state){
    tagtwid->setVisible(state);
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
    for(auto ci:schedulelw->selectionModel()->selectedIndexes()){
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
}
void pgWrite::onItemMoveXCoord(double value){
    for(auto ci:schedulelw->selectionModel()->selectedIndexes()){
        if(!ci.isValid()) return;
        for(std::vector<schItem>::iterator it=scheduled.begin();it!=scheduled.end();++it){
            if(it->ptr==schedulemod->item(ci.row(),0)){
                it->coords[0]+=value*1e-7;
                if(it->isWrite) updateItemCoord(it);
                break;
            }
        }
    }
}
void pgWrite::onItemMoveYCoord(double value){
    for(auto ci:schedulelw->selectionModel()->selectedIndexes()){
        if(!ci.isValid()) return;
        for(std::vector<schItem>::iterator it=scheduled.begin();it!=scheduled.end();++it){
            if(it->ptr==schedulemod->item(ci.row(),0)){
                it->coords[1]+=value*1e-7;
                if(it->isWrite) updateItemCoord(it);
                break;
            }
        }
    }
}
void pgWrite::updateItemCoord(std::vector<schItem>::iterator it){
    ovl.rm_overlay(it->overlay);
    cv::Mat resized;
    double ratio=it->wps.imgmmPPx*1000;
    double xSize=round(ratio*it->src.cols)*1000/pgMGUI->getNmPPx(0);
    double ySize=round(ratio*it->src.rows)*1000/pgMGUI->getNmPPx(0);
    cv::resize(it->src, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
    it->overlay=ovl.add_overlay(resized,pgMGUI->mm2px(it->coords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(it->coords[1]-pgBeAn->writeBeamCenterOfsY,0));
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
    if(refocusBeforeScan->val) pgFGUI->doRefocus(true, ROI, maxRedoRefocusTries);
    if(pgSGUI->doNRounds(scanRepeatN->val,ROI.width==0?scanROI:ROI,maxRedoScanTries)) return true;
    res=scanRes->get();
    if(res==nullptr){std::cerr<<"Somehow cannot find scan in pgWrite::_onScan()\n";return true;}
    return false;
}
void pgWrite::onSave(){
    _onSave(true, "", genConfig());
}
bool pgWrite::_onSave(bool ask, std::string filename, std::string config){
    res=scanRes->get();
    if(res==nullptr){saveB->setEnabled(false);return true;}

    if(filename=="") filename=filenaming->get();
    else if(filename[0]=='/') goto skipFn;
    replacePlaceholdersInString(filename);
    {std::string path=util::toString(scan_default_folder->get(),"/",filename);
    filename=path;
    size_t found=path.find_last_of("/");
    if(found!=std::string::npos){
        path.erase(found,path.size()+found);
        cv::utils::fs::createDirectories(path);
    }}
    skipFn:;
    if(ask){
        filename=QFileDialog::getSaveFileName(gui_activation,"Save scan to file.",QString::fromStdString(filename),"Images (*.pfm)").toStdString();
        if(filename.empty()) return true;
    }
    pgScanGUI::saveScan(res,filename);
    if(!config.empty()) saveConfig(filename, config);
    return false;
}
void pgWrite::saveConfig(std::string filename, std::string config){
    if(filename.size()>4){
        if(filename.substr(filename.size()-4)==".pfm")
            filename.replace(filename.size()-4,4,".cfg");
        else filename+=".cfg";
    }
    std::ofstream wfile(filename);
    wfile<<config;
    wfile.close();
}
std::string pgWrite::genConfig(){
    std::string fn=fileName;
    size_t found=fn.find_last_of("/");
    if(found!=std::string::npos) fn.erase(0,found+1);
    std::string ret= util::toString(
                "Tag String: ",tagString->text().toStdString(),"\n",
                "Tag UInt: ",static_cast<int>(tagUInt->val),"\n",
                "MaxDepth: ",lastDepth," nm\n",
                "ImgUmPPx: ",imgUmPPx->val,"\n",
                "Source filename: ",fn,"\n"
                );
    if(gradualWEn->val) ret+=util::toString("Gradual enabled. Method: ",gradualWCut->val?"Cut":"Scale",". Step: ",gradualW->val," nm\n");
    //TODO save all write parameters (implment mixed scheduling first!)
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
void pgWrite::onPulse(){
    if(!go.pCTRL->isConnected()) return;
    wasMirau=pgMGUI->currentObj==0;
    pgMGUI->chooseObj(false);    // switch to writing
    double pulse, vfocus{0}, vfocusXcor{0}, vfocusYcor{0};
    if(pulseDur->index==0) pulse=pulseDur->val;
    else{
        preparePredictor();
        pulse=predictDuration(pulseDur->val);
    }
    vfocus=focus->val/1000;
    vfocusXcor=focusXcor->val/1000;
    vfocusYcor=focusYcor->val/1000;
    CTRL::CO co(go.pCTRL);
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
    while(go.pCTRL->getMotionSetting("",CTRL::mst_position_update_pending)!=0) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    pgMGUI->getPos(&scanCoords[0], &scanCoords[1], &scanCoords[2]);

    cv::Mat _WRImage=WRImage.clone();
    if(rotation->val!=0) rotateIm(&_WRImage);
    prepareScanROI(_WRImage, imgUmPPx->val);
    if(_WRImage.type()!=CV_32F) lastDepth=depthMaxval->val;
    else cv::minMaxIdx(_WRImage,0,&lastDepth);

    if(!firstWritten)firstWritten=true;
    else tagUInt->setValue(tagUInt->val+1);

    if(useWriteScheduling->val){    // scheduling
        std::string filename=filenaming->get();
        replacePlaceholdersInString(filename);
        scheduled.emplace_back();
        for(int i:{0,1,2})scheduled.back().coords[i]=scanCoords[i];
        _WRImage.copyTo(scheduled.back().src);
        scheduled.back().isWrite=true;
        scheduled.back().wps.depthMaxval=depthMaxval->val;
        scheduled.back().wps.imgmmPPx=imgUmPPx->val/1000;
        scheduled.back().wps.pointSpacing_mm=pointSpacing->val/1000;
        scheduled.back().wps.focus_mm=focus->val/1000;
        scheduled.back().wps.focusXcor_mm=focusXcor->val/1000;
        scheduled.back().wps.focusYcor_mm=focusYcor->val/1000;
        scheduled.back().wps.depthScale=depthScale->val;
        scheduled.back().wps.gradualW=(gradualWEn->val)?((double)gradualW->val):(0);
        scheduled.back().wps.gradualWCut=gradualWCut->val;
        scheduled.back().ptr=addScheduleItem("Pending","Write",filename,false);
        cv::Mat resized;
        double ratio=imgUmPPx->val;
        double xSize=round(ratio*_WRImage.cols)*1000/pgMGUI->getNmPPx(0);
        double ySize=round(ratio*_WRImage.rows)*1000/pgMGUI->getNmPPx(0);
        if(xSize>0 && ySize>0){
             cv::resize(_WRImage, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
             scheduled.back().overlay=ovl.add_overlay(resized,pgMGUI->mm2px(scanCoords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(scanCoords[1]-pgBeAn->writeBeamCenterOfsY,0));
        }

        if(scheduleScans->val){
            for(auto& ROI:scanROIs){
                scheduled.emplace_back();
                for(int i:{0,1,2})scheduled.back().coords[i]=ROI.scanCoords[i];
                scheduled.back().isWrite=false;
                scheduled.back().filename=filename+ROI.filenameadd;
                scheduled.back().conf=genConfig();
                scheduled.back().scanROI=ROI.ROI;
                scheduled.back().ptr=addScheduleItem("Pending","Scan",filename+ROI.filenameadd,false);
            }
        }

        return;
    }

    scanB->setEnabled(false);
    saveB->setEnabled(false);
    writeDM->setText("Abort");
    onChangeDrawWriteAreaOn(false);
    //write:
    if(refocusBeforeWrite->val){
        if(scanROIs.size()==1) pgFGUI->doRefocus(true, scanROI, maxRedoRefocusTries);
        else pgFGUI->doRefocus(true, {0,0,0,0}, maxRedoRefocusTries);
    }
    if(writeMat()) firstWritten=false;

    if(firstWritten){
        scanB->setEnabled(scanROIs.size()==1);
        scanB->setToolTip(scanROIs.size()==1?"":"For large (multiple) scans use scheduling!");
        writeDM->setEnabled(true);
    }
    writeDM->setText("Write");
    writeDM->setEnabled(true);
}
void pgWrite::scheduleWriteScan(cv::Mat& src, writePars pars, std::string scanSaveFilename, std::string notes){
    // save coords for scan
    while(go.pCTRL->getMotionSetting("",CTRL::mst_position_update_pending)!=0) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);

    cv::Mat srcm=src.clone();
    scheduleWrite(srcm, pars, scanSaveFilename);
    scheduleScan(srcm, pars.imgmmPPx,scanSaveFilename, notes, false);
}
void pgWrite::scheduleWrite(cv::Mat& src, writePars pars, std::string label){
    pgMGUI->getPos(&scanCoords[0], &scanCoords[1], &scanCoords[2]);

    scheduled.emplace_back();
    for(int i:{0,1,2})scheduled.back().coords[i]=scanCoords[i];
    src.copyTo(scheduled.back().src);
    scheduled.back().isWrite=true;
    scheduled.back().wps=pars;
    scheduled.back().ptr=addScheduleItem("Pending","Write",label,false);
    cv::Mat resized;
    double ratio=pars.imgmmPPx*1000;
    double xSize=round(ratio*src.cols)*1000/pgMGUI->getNmPPx(0);
    double ySize=round(ratio*src.rows)*1000/pgMGUI->getNmPPx(0);
    if(xSize>0 && ySize>0){
        cv::resize(src, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
        scheduled.back().overlay=ovl.add_overlay(resized,pgMGUI->mm2px(scanCoords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(scanCoords[1]-pgBeAn->writeBeamCenterOfsY,0));
    }
}
void pgWrite::scheduleScan(cv::Mat& src, double imgmmPPx, std::string scanSaveFilename, std::string notes, bool getpos){
    if(getpos) pgMGUI->getPos(&scanCoords[0], &scanCoords[1], &scanCoords[2]);
    prepareScanROI(src, imgmmPPx*1000);
    for(auto& ROI:scanROIs){
        scheduled.emplace_back();
        for(int i:{0,1,2})scheduled.back().coords[i]=ROI.scanCoords[i];
        scheduled.back().isWrite=false;
        scheduled.back().filename=scanSaveFilename+ROI.filenameadd;
        scheduled.back().conf=notes;
        scheduled.back().scanROI=ROI.ROI;
        scheduled.back().ptr=addScheduleItem("Pending","Scan",scanSaveFilename,false);
    }
}
double pgWrite::getScanExtraBorderPx(){
    double extraB=scanExtraBorder->val;
    if(extraB>0){
        if(scanExtraBorder->index==0){  // um
            extraB=pgMGUI->mm2px(extraB/1000,0);
        }
    }
    return extraB;
}
void pgWrite::prepareScanROI(cv::Mat& mat, double _imgUmPPx){
    int xSize=pgMGUI->mm2px(round(_imgUmPPx*mat.cols+2)/1000,0);
    int ySize=pgMGUI->mm2px(round(_imgUmPPx*mat.rows+2)/1000,0);
    int cols=go.pGCAM->iuScope->camCols;
    int rows=go.pGCAM->iuScope->camRows;

    double extraB=getScanExtraBorderPx();
    scanROIs.clear();
    scanROI=cv::Rect(cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX,0)-extraB, rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY,0)-extraB, xSize+2*extraB, ySize+2*extraB);
    if(scanROI.x>=0 && scanROI.y>=0 && scanROI.width+scanROI.x<=cols && scanROI.height+scanROI.y<=rows){
        scanROIs.emplace_back();
        scanROIs.back().ROI=scanROI;
        scanROIs.back().filenameadd="";
        for(auto i:{0,1,2}) scanROIs.back().scanCoords[i]=scanCoords[i];
    }else{
        int Ncols=ceil((double)xSize/cols);
        int Nrows=ceil((double)ySize/rows);
        int xSizeSeg=xSize/Ncols+2*extraB;
        int ySizeSeg=ySize/Nrows+2*extraB;
        cv::Rect newROI=cv::Rect(cols/2-xSizeSeg/2, rows/2-ySizeSeg/2, xSizeSeg, ySizeSeg);
        for(int row=0;row!=Nrows;row++){
            for(int col=0;col!=Ncols;col++){
                scanROIs.emplace_back();
                scanROIs.back().ROI=newROI;
                scanROIs.back().filenameadd=util::toString("_y",row,"_x",col);
                for(auto i:{0,1,2}) scanROIs.back().scanCoords[i]=scanCoords[i];
                scanROIs.back().scanCoords[0]+=pgMGUI->px2mm(-0.5*xSize+(0.5+col)*xSize/Ncols);
                scanROIs.back().scanCoords[1]+=pgMGUI->px2mm(-0.5*ySize+(0.5+row)*ySize/Nrows);
            }
        }
    }
}
bool pgWrite::writeMat(cv::Mat* override, writePars wparoverride){
    wabort=false;
    cv::Mat tmpWrite, resizedWrite;
    cv::Mat* src;
    double vdepthMaxval, vimgmmPPx, vpointSpacing, vfocus, vfocusXcor, vfocusYcor, vdepthScale;

    src=(override!=nullptr)?override:&WRImage;
    vdepthMaxval=(wparoverride.depthMaxval!=0)?wparoverride.depthMaxval:(depthMaxval->val.load());
    vimgmmPPx=(wparoverride.imgmmPPx!=0)?wparoverride.imgmmPPx:imgUmPPx->val/1000;
    vpointSpacing=(wparoverride.pointSpacing_mm!=0)?wparoverride.pointSpacing_mm:pointSpacing->val/1000;
    vfocus=(wparoverride.focus_mm!=std::numeric_limits<double>::max())?wparoverride.focus_mm:focus->val/1000;
    vfocusXcor=(wparoverride.focusXcor_mm!=std::numeric_limits<double>::max())?wparoverride.focusXcor_mm:focusXcor->val/1000;
    vfocusYcor=(wparoverride.focusYcor_mm!=std::numeric_limits<double>::max())?wparoverride.focusYcor_mm:focusYcor->val/1000;
    vdepthScale=(wparoverride.depthScale>0)?wparoverride.depthScale:(depthScale->val.load());
    double vgradualW; bool vgradualWCut;
    vgradualW=(wparoverride.gradualW<0)?((gradualWEn->val)?((double)gradualW->val):(0)):wparoverride.gradualW;
    vgradualWCut=(wparoverride.gradualWCut<0)?(bool)gradualWCut->val:(bool)wparoverride.gradualWCut;
    bool noCalib=wparoverride.matrixIsDuration;

    if(src->type()==CV_8U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/255);
    }else if(src->type()==CV_16U){
        src->convertTo(tmpWrite, CV_32F, vdepthMaxval/65536);
    }else if(src->type()==CV_32F){
        src->copyTo(tmpWrite);
        if(vdepthScale!=1)
            tmpWrite*=vdepthScale;
    }else {QMessageBox::critical(gui_activation, "Error", "Image type not compatible.\n"); return true;}
    if(override==nullptr && rotation->val!=0) rotateIm(&tmpWrite);

    preparePredictor();

    double ratio=vimgmmPPx/vpointSpacing;
    try{
        if(vimgmmPPx==vpointSpacing) tmpWrite.copyTo(resizedWrite);
        else if(vimgmmPPx<vpointSpacing) cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_AREA);   //shrink
        else cv::resize(tmpWrite, resizedWrite, cv::Size(0,0),ratio,ratio, cv::INTER_CUBIC);                         //enlarge
    }
    catch (const std::exception& e){
        QMessageBox::critical(gui_activation, "Error", "Image has zero rows/columns after resize according to umppx and point spacing (ie. write structure too small).\n");
        return true;
    }

    // at this point resizedWrite contains the desired depth at each point
    std::lock_guard<std::mutex>lock(MLP._lock_proc);
    if(!go.pCTRL->isConnected()) return true;
    wasMirau=pgMGUI->currentObj==0;
    pgMGUI->chooseObj(false);    // switch to writing

    std::vector<cv::Mat> writes;
    if(vgradualW<=0) writes.push_back(resizedWrite);
    else{
        double max;
        int n;
        cv::minMaxIdx(resizedWrite,nullptr,&max);
        n=ceil(max/vgradualW);
        if(vgradualWCut){
            cv::Mat mask;
            for(int i=0;i<n;i++){
                writes.emplace_back();
                resizedWrite.copyTo(writes.back());
                cv::compare(resizedWrite,i*vgradualW,mask,cv::CMP_LT);
                writes.back().setTo(0,mask);
                cv::compare(resizedWrite,(i+1)*vgradualW,mask,cv::CMP_GT);
                writes.back().setTo((i+1)*vgradualW,mask);
            }
        }else{
            for(int i=0;i<n;i++){
                writes.emplace_back();
                resizedWrite.copyTo(writes.back());
                if((i+1)*vgradualW<max)
                    writes.back()*=((i+1)*vgradualW/max);
            }
        }
    }

    for(int wr=0;wr<writes.size();wr++){
        auto& write=writes[wr];

        CTRL::CO CO(go.pCTRL);
        double offsX,offsY;
        offsX=(write.cols-1)*vpointSpacing;
        offsY=(write.rows-1)*vpointSpacing;
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
            for(int j=0;j!=write.rows;j++){   // write row by row (so that processing for the next row is done while writing the previous one, and the operation can be terminated more easily)
                for(int i=0;i!=write.cols;i++){
                    if(noCalib) pulse=write.at<float>(write.rows-j-1,xdir?(write.cols-i-1):i);
                    else pulse=predictDuration(write.at<float>(write.rows-j-1,xdir?(write.cols-i-1):i));        // Y inverted because image formats have flipped Y
                    if(i!=0) pgMGUI->corCOMove(CO,(xdir?-1:1)*vpointSpacing,0,0);
                    if(writeZeros->val==false && write.at<float>(write.rows-j-1,xdir?(write.cols-i-1):i)==0) continue;
                    if(pulse==0) continue;
                    CO.addHold("X",CTRL::he_motion_ontarget);
                    CO.addHold("Y",CTRL::he_motion_ontarget);
                    CO.addHold("Z",CTRL::he_motion_ontarget);   // because corCOMove may correct Z too
                    CO.pulseGPIO("wrLaser",pulse/1000);
                }
                if (j!=write.rows-1) pgMGUI->corCOMove(CO,0,vpointSpacing,0);
                CO.execute();
                CO.clear(true);
                xdir^=1;

                while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
                //MLP.progress_proc=100./write.rows*j;
                MLP.progress_proc=100./(write.rows*writes.size())*(j+wr*write.rows);
                if(wabort) {pgMGUI->move(0,0,-vfocus); if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true); return true;}
            }

            pgMGUI->corCOMove(CO,-vfocusXcor,-vfocusYcor,-vfocus);
            pgMGUI->corCOMove(CO,(xdir?-1:1)*offsX/2,-offsY/2,0);
            CO.execute();
            CO.clear(true);
        }else if(selPScheduling->index==1 ||selPScheduling->index==2 ){ //Nearest or Spiral
            cv::flip(write,write,0);
            cv::Mat completed=cv::Mat::zeros(write.size(), CV_8U);
            if(writeZeros->val==false) cv::compare(write,0,completed,cv::CMP_EQ);
            cv::Mat pulseMat=cv::Mat::zeros(write.size(), CV_32F);
            for(int j=0;j!=write.rows;j++) for(int i=0;i!=write.cols;i++){
                if(noCalib) pulseMat.at<float>(j,i)=write.at<float>(j,i);
                else pulseMat.at<float>(j,i)=predictDuration(write.at<float>(j,i));
            }
            int last_i=0, last_j=0;
            int next_i=last_i, next_j=last_j;

            long todo=completed.rows*completed.cols-cv::countNonZero(completed);
            const long total=completed.rows*completed.cols-cv::countNonZero(completed);
            if(todo==0) goto abort;

            struct pt{
                int i,j;
                double distance;    // in um
            };
            std::vector<pt> lut;
            const double velX=go.pCTRL->getMotionSetting("X",CTRL::mst_defaultVelocity);
            const double velY=go.pCTRL->getMotionSetting("Y",CTRL::mst_defaultVelocity);
            const double accX=go.pCTRL->getMotionSetting("X",CTRL::mst_defaultAcceleration);
            const double accY=go.pCTRL->getMotionSetting("Y",CTRL::mst_defaultAcceleration);
            double timeX,timeY;
            size_t iter;

            if(selPScheduling->index==1){   //Nearest
                for(int j=0;j!=write.rows;j++) for(int i=0;i!=write.cols;i++){
                    mcutil::evalAccMotion(i*vpointSpacing, accX, velX, &timeX);
                    mcutil::evalAccMotion(j*vpointSpacing, accY, velY, &timeY);
                    lut.push_back({i,j,sqrt(pow(i*vpointSpacing,2)+pow(j*vpointSpacing,2))});
                }
                std::sort(lut.begin(), lut.end(), [](pt i,pt j){return (i.distance<j.distance);});

                if(completed.at<uint8_t>(next_j,next_i)==255) for(auto& el: lut)
                    if(completed.at<uint8_t>(el.j,el.i)==0){
                        next_j=el.j;
                        next_i=el.i;
                        break;
                    }
            }else if(selPScheduling->index==2){ // Spiral
                std::deque<pt> lut2;
                for(int j=0;j!=write.rows;j++) for(int i=0;i!=write.cols;i++){
                    if(completed.at<uint8_t>(j,i)==0)
                        lut2.push_back({i,j,sqrt(pow((i-0.5*write.cols)*vpointSpacing,2)+pow((j-0.5*write.rows)*vpointSpacing,2))});
                }

                std::sort(lut2.begin(), lut2.end(), [](pt i,pt j){return (i.distance<j.distance);});
                // purely sorted by distance is slow as it will jump around, we want a closest in a ring
                const double ringwidth=3*vpointSpacing;

                lut.push_back(lut2.front());
                lut2.pop_front();
                std::deque<pt> tmp;
                while(!lut2.empty()){
                    while(abs(lut2.front().distance-lut.back().distance)<ringwidth){
                        tmp.push_back(lut2.front());
                        lut2.pop_front();
                        if(lut2.empty()) break;
                    }
                    if(tmp.empty() && !lut2.empty()){
                        lut.push_back(lut2.front());
                        lut2.pop_front();
                        continue;
                    }
                    std::sort(tmp.begin(), tmp.end(), [&cur = std::as_const(lut.back())](pt i,pt j){
                        return (sqrt(pow((cur.i-i.i),2)+pow((cur.j-i.j),2))<sqrt(pow((cur.i-j.i),2)+pow((cur.j-j.j),2)));
                    });
                    lut.push_back(tmp.front());
                    tmp.pop_front();
                    while(!tmp.empty()){
                        lut2.push_front(tmp.front());
                        tmp.pop_front();
                    }
                }

                iter=0;
                next_i=lut[0].i;
                next_j=lut[0].j;
            }

            int nPerRun=sqrt(write.rows*write.cols);
            if(nPerRun<100) nPerRun=100;
            while(1){
                for(int i=0;i!=nPerRun; i++){
                    pgMGUI->corCOMove(CO,(next_i-last_i)*vpointSpacing,(next_j-last_j)*vpointSpacing,0);
                    last_i=next_i;
                    last_j=next_j;
                    CO.addHold("X",CTRL::he_motion_ontarget);
                    CO.addHold("Y",CTRL::he_motion_ontarget);
                    CO.addHold("Z",CTRL::he_motion_ontarget);   // because corCOMove may correct Z too
                    if(noCalib) CO.pulseGPIO("wrLaser",write.at<float>(next_j,next_i)/1000);
                    else CO.pulseGPIO("wrLaser",predictDuration(write.at<float>(next_j,next_i))/1000);
                    completed.at<uint8_t>(next_j,next_i)=255;
                    todo--;

                    if(todo==0) break;
                    if(selPScheduling->index==1){   //Nearest
                        for(auto& el: lut) for(int tmp_i:{last_i-el.i,last_i+el.i}) for(int tmp_j:{last_j-el.j,last_j+el.j}){     // there is some redundancy
                            if(tmp_i<0 || tmp_i>=completed.cols || tmp_j<0 || tmp_j>=completed.rows) continue;
                            if(completed.at<uint8_t>(tmp_j,tmp_i)==0){
                                next_i=tmp_i;
                                next_j=tmp_j;
                                goto next;
                            }
                        }
                    }else if(selPScheduling->index==2){ // Spiral
                        iter++;
                        if(iter>lut.size()) break;  // should be redundant
                        next_j=lut[iter].j;
                        next_i=lut[iter].i;
                        continue;
                    }
                    QMessageBox::critical(gui_activation, "Error", "Cannot find a point in pgWrite::writeMat; this shouldn't happen!"); goto abort; // in case there is an unforseen bug, TODO remove
                    next:;
                }
                CO.execute();
                CO.clear(true);
                while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
                //MLP.progress_proc=100./total*(total-todo);
                MLP.progress_proc=100./writes.size()*wr+100./writes.size()/total*(total-todo);
                if(wabort) {pgMGUI->move(0,0,-vfocus); if(wasMirau&&switchBack2mirau->val)pgMGUI->chooseObj(true); return true;}
                if(todo==0) break;
            }

            pgMGUI->corCOMove(CO,-vfocusXcor,-vfocusYcor,-vfocus);
            pgMGUI->corCOMove(CO,-last_i*vpointSpacing+offsX/2,-last_j*vpointSpacing+offsY/2,0);
            CO.execute();
            CO.clear(true);
        }
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
    if(rotation->val!=0) rotateIm(&tagImage);

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
        scheduled.back().wps.depthMaxval=settingWdg[4]->depthMaxval->val;
        scheduled.back().wps.imgmmPPx=settingWdg[4]->imgUmPPx->val/1000;
        scheduled.back().wps.pointSpacing_mm=settingWdg[4]->pointSpacing->val/1000;
        scheduled.back().wps.focus_mm=settingWdg[4]->focus->val/1000;
        scheduled.back().wps.focusXcor_mm=settingWdg[4]->focusXcor->val/1000;
        scheduled.back().wps.focusYcor_mm=settingWdg[4]->focusYcor->val/1000;
        scheduled.back().wps.depthScale=-1;
        scheduled.back().wps.gradualW=0;
        scheduled.back().wps.gradualWCut=-1;
        scheduled.back().wps.matrixIsDuration=true;  // TODO fix, this is a dirty fix
        scheduled.back().ptr=addScheduleItem("Pending","Write",util::toString("Tag: ",tagText->text().toStdString()),false);
        cv::Mat resized;
        double ratio=settingWdg[4]->imgUmPPx->val;
        double xSize=round(ratio*tagImage.cols)*1000/pgMGUI->getNmPPx(0);
        double ySize=round(ratio*tagImage.rows)*1000/pgMGUI->getNmPPx(0);
        if(xSize>0 && ySize>0){
            cv::resize(tagImage, resized, cv::Size(xSize, ySize), cv::INTER_LINEAR);
            scheduled.back().overlay=ovl.add_overlay(resized,pgMGUI->mm2px(coords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(coords[1]-pgBeAn->writeBeamCenterOfsY,0));
        }
    }
    else{
        writeTag->setText("Abort");
        onChangeDrawWriteAreaOnTag(false);
        writePars wps;
        wps.depthMaxval=settingWdg[4]->depthMaxval->val;
        wps.imgmmPPx=settingWdg[4]->imgUmPPx->val/1000;
        wps.pointSpacing_mm=settingWdg[4]->pointSpacing->val/1000;
        wps.focus_mm=settingWdg[4]->focus->val/1000;
        wps.focusXcor_mm=settingWdg[4]->focusXcor->val/1000;
        wps.focusYcor_mm=settingWdg[4]->focusYcor->val/1000;
        wps.depthScale=-1;
        wps.gradualW=0;
        wps.gradualWCut=-1;
        wps.matrixIsDuration=true;  // TODO fix, this is a dirty fix
        // TODO tag doesnt use all the tag settings, fix when variable scheduling is implemented
        writeMat(&tagImage,wps);
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
    return T;
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
void pgWrite::changeDrawAreaOnExternal(bool status, double xsize_um, double ysize_um){
    ext_xsize_um=xsize_um;
    ext_ysize_um=ysize_um;
    drawWriteAreaOn=status?4:0;
}
void pgWrite::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double ratio;
    double xSize;
    double ySize;
    double xShiftmm=0;
    double yShiftmm=0;
    double rot=0;

    if(drawWriteAreaOn==2){ //tag
        if(tagText->text().toStdString().empty()) return;
        ratio=settingWdg[4]->imgUmPPx->val;
        cv::Size size=cv::getTextSize(tagText->text().toStdString(), OCV_FF::ids[settingWdg[4]->fontFace->index], settingWdg[4]->fontSize->val, settingWdg[4]->fontThickness->val, nullptr);
        rot=-rotation->val/180*M_PI;
        xSize=round(ratio*(size.width+4))*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*(size.height+4))*1000/pgMGUI->getNmPPx();
    }else if(drawWriteAreaOn==1){ // write
        if(WRImage.empty() || !writeDM->isEnabled()) return;
        ratio=imgUmPPx->val;
        rot=-rotation->val/180*M_PI;
        xSize=round(ratio*WRImage.cols)*1000/pgMGUI->getNmPPx();
        ySize=round(ratio*WRImage.rows)*1000/pgMGUI->getNmPPx();
    }else if(drawWriteAreaOn==3){  // scan
        if(!scanB->isEnabled() || !go.pCTRL->isConnected()) return;
        pgMGUI->getPos(&xShiftmm, &yShiftmm);
        xShiftmm-=scanCoords[0];
        yShiftmm-=scanCoords[1];
        xSize=pgMGUI->mm2px(pgMGUI->px2mm(scanROI.width,0));
        ySize=pgMGUI->mm2px(pgMGUI->px2mm(scanROI.height,0));
    }else if(drawWriteAreaOn==4){ // external
        xSize=round(ext_xsize_um)*1000/pgMGUI->getNmPPx();
        ySize=round(ext_ysize_um)*1000/pgMGUI->getNmPPx();
    }else if(drawWriteAreaOn==5){ // scheduled item hover
        xSize=pgMGUI->mm2px(drawAreaScheduledRect[2]);
        ySize=pgMGUI->mm2px(drawAreaScheduledRect[3]);
        pgMGUI->getPos(&xShiftmm, &yShiftmm);
        xShiftmm-=drawAreaScheduledRect[0];
        yShiftmm-=drawAreaScheduledRect[1];
    }

    double clr[2]={0,255}; int thck[2]={3,1};
    std::vector<cv::Point2f> corners;
    for(int i:{-1,1}) for(int j:{-1,1})
        corners.push_back({(float)(sqrt(pow(0.5*xSize,2)+pow(0.5*ySize,2))),(float)(rot+atan2(i*0.5*ySize,j*0.5*xSize))});
    double ofsx=img->cols/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX+xShiftmm);
    double ofsy=img->rows/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY+yShiftmm);

    for(auto& cr: corners) cr=cv::Point2f(ofsx+cr.x*cos(cr.y),ofsy+cr.x*sin(cr.y));
    std::iter_swap(corners.begin()+2, corners.begin()+3);
    for(int i=0;i!=2;i++)
        for(int j=0;j!=4;j++)
            cv::line(*img, corners[j], corners[(j+1)%4], {clr[i]}, thck[i], cv::LINE_AA);
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
                if(refocusBeforeWrite->val){
                    if(scanROIs.size()==1) pgFGUI->doRefocus(true, scanROI, maxRedoRefocusTries);
                    else pgFGUI->doRefocus(true, {0,0,0,0}, maxRedoRefocusTries);
                }
                failed=writeMat(&it->src, it->wps);
            }else{
                saveB->setEnabled(false);
                failed=_onScan(it->scanROI, it->coords);
                if(!failed){
                    if(_onSave(false, it->filename, it->conf)){
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

void pgWrite::setScheduling(bool value){
    useWriteScheduling->set(value);
}
void pgWrite::rotateIm(cv::Mat* mat){
    cv::Mat tmp=mat->clone();
    double angle=std::fmod(rotation->val,360);
    cv::Size size=calcRotSize(mat->size(), angle/180*M_PI);
    cv::Mat TM=cv::getRotationMatrix2D(cv::Point2f(tmp.cols*0.5,tmp.rows*0.5), angle, 1.0);
    TM.at<double>(0,2)+=0.5*(size.width-tmp.cols);
    TM.at<double>(1,2)+=0.5*(size.height-tmp.rows);
    cv::warpAffine(tmp, *mat, TM, size);
}
cv::Size pgWrite::calcRotSize(cv::Size size, double angleRad){
    struct pa{double amp; double an;/*rad*/};
    std::vector<pa> corners;
    for(int i:{-1,1}) for(int j:{-1,1})
        corners.push_back({sqrt(pow(0.5*size.width,2)+pow(0.5*size.height,2)),angleRad+atan2(i*0.5*size.height,j*0.5*size.width)});
    cv::Size ret{0,0};
    for(auto ed: corners){
        if(2*abs(ed.amp*cos(ed.an))>ret.width) ret.width=2*abs(ed.amp*cos(ed.an));
        if(2*abs(ed.amp*sin(ed.an))>ret.height) ret.height=2*abs(ed.amp*sin(ed.an));
    }
    return ret;
}
void pgWrite::onHoverOverScheduledItem(const QModelIndex &index){
    for(auto& item:scheduled) if(item.ptr==schedulemod->item(index.row(), 0)){
        if(item.isWrite){
            drawAreaScheduledRect[2]=item.src.cols*item.wps.imgmmPPx;
            drawAreaScheduledRect[3]=item.src.rows*item.wps.imgmmPPx;
        }else{
            drawAreaScheduledRect[2]=pgMGUI->px2mm(item.scanROI.width,0);
            drawAreaScheduledRect[3]=pgMGUI->px2mm(item.scanROI.height,0);
        }
        drawAreaScheduledRect[0]=item.coords[0];
        drawAreaScheduledRect[1]=item.coords[1];
        break;
    }
    drawWriteAreaOn=5;
}
void leQTreeView::leaveEvent(QEvent *event){
    parent->drawWriteAreaOn=0;
}
