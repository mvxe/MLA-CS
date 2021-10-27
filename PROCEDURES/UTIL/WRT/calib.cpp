#include "calib.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/focus.h"
#include "PROCEDURES/UTIL/USC/move.h"
#include "PROCEDURES/UTIL/WRT/bounds.h"
#include "opencv2/core/utils/filesystem.hpp"
#include "GUI/tab_monitor.h"    //for debug purposes
#include <dirent.h>
#include <sys/stat.h>
#include <dlib/optimization.h>
#include <algorithm>
#include <random>

pgCalib::pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv, pgBeamAnalysis* pgBeAn, pgWrite* pgWr): pgSGUI(pgSGUI), pgBGUI(pgBGUI), pgFGUI(pgFGUI), pgMGUI(pgMGUI), pgDpEv(pgDpEv), pgBeAn(pgBeAn), pgWr(pgWr){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);

    btnWriteCalib=new HQPushButton("Calibrate Write Focus");
    connect(btnWriteCalib, SIGNAL(released()), this, SLOT(onWCF()));
    connect(btnWriteCalib, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    btnWriteCalib->setCheckable(true);
    alayout->addWidget(new twid(btnWriteCalib));


    connect(pgDpEv, SIGNAL(sigGoToNearestFree(double, double, double, double, double, bool)), this, SLOT(goToNearestFree(double, double, double, double, double, bool)));

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    selArrayXsize=new val_selector(10, "Array Size X", 1, 1000, 0);
    conf["selArrayXsize"]=selArrayXsize;
    slayout->addWidget(selArrayXsize);
    selArrayYsize=new val_selector(10, "Array Size Y", 1, 1000, 0);
    conf["selArrayYsize"]=selArrayYsize;
    slayout->addWidget(selArrayYsize);
    selArraySpacing=new val_selector(5, "Array Spacing", 0.001, 100, 3, 0, {"um"});
    conf["selArraySpacing"]=selArraySpacing;
    slayout->addWidget(selArraySpacing);
    selArrayType=new smp_selector("Variable Parameters (X-Y): ", 0, {"Duration(X,Y), no repeat","Focus(X,Y), no repeat","Duration(X)-Focus(Y)", "Duration(X,Y), repeat","Focus(X,Y), repeat"});
    connect(selArrayType, SIGNAL(changed(int)), this, SLOT(onSelArrayTypeChanged(int)));
    conf["selArrayType"]=selArrayType;
    slayout->addWidget(selArrayType);
    transposeMat=new checkbox_gs(false,"Transpose matrix.");
    conf["transposeMat"]=transposeMat;
    slayout->addWidget(transposeMat);
    slayout->addWidget(new QLabel("The Variable Parameters Will be Within the Specified Range."));
    selArrayDurA=new val_selector(1, "Duration", 0.001, 1000, 3, 0, {"ms"});
    conf["selArrayDurA"]=selArrayDurA;
    slayout->addWidget(selArrayDurA);
    selArrayDurB=new val_selector(1, "Duration upper limit", 0.001, 1000, 3, 0, {"ms"});
    conf["selArrayDurB"]=selArrayDurB;
    slayout->addWidget(selArrayDurB);
    selArrayFocA=new val_selector(-1, "Focus", -1000, 1000, 3, 0, {"um"});
    conf["selArrayFocA"]=selArrayFocA;
    slayout->addWidget(selArrayFocA);
    selArrayFocB=new val_selector(1, "Focus upper limit", -1000, 1000, 3, 0, {"um"});
    conf["selArrayFocB"]=selArrayFocB;
    slayout->addWidget(selArrayFocB);
    selArrayOneScanN=new val_selector(10, "Take and average This Many Scans: ", 1, 1000, 0);
    conf["selArrayOneScanN"]=selArrayOneScanN;
    slayout->addWidget(selArrayOneScanN);
    selArrayRandomize=new checkbox_gs(false,"Randomize Value Order");
    conf["selArrayRandomize"]=selArrayRandomize;
    slayout->addWidget(selArrayRandomize);
    saveMats=new checkbox_gs(true,"Extra Save Mats Containing D,F for Convenience.");
    conf["saveMats"]=saveMats;
    slayout->addWidget(saveMats);
    savePic=new checkbox_gs(true,"Also save direct pictures of measurements.");
    conf["savePic"]=savePic;
    slayout->addWidget(savePic);
    slayout->addWidget(new hline);
    selPrerunType=new smp_selector("Prerun type ", 0, {"Plateau","Peak"});
    connect(selPrerunType, SIGNAL(changed(int)), this, SLOT(onPrerunTypeChanged(int)));
    conf["selPrerunType"]=selPrerunType;
    slayout->addWidget(selPrerunType);
    selPlateauA=new val_selector(0, "Height", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauA"]=selPlateauA;
    slayout->addWidget(selPlateauA);
    selPlateauB=new val_selector(0, "Height upper limit", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauB"]=selPlateauB;
    slayout->addWidget(selPlateauB);
    selPeakXshift=new val_selector(0, "Peak shift X", -100, 100, 3, 0, {"um"});
    conf["selPeakXshift"]=selPeakXshift;
    slayout->addWidget(selPeakXshift);
    selPeakYshift=new val_selector(0, "Peak shift Y", -100, 100, 3, 0, {"um"});
    conf["selPeakYshift"]=selPeakYshift;
    slayout->addWidget(selPeakYshift);
    slayout->addWidget(new QLabel("NOTE: baseline calibration needed for plateau/peak."));
    slayout->addWidget(new QLabel("NOTE: set height to 0 to disable."));
    slayout->addWidget(new hline);
    selMultiArrayType=new smp_selector("Multiple runs variable parameter: ", 0, {"none","Focus","Duration", "Plateau/Peak"});
    connect(selMultiArrayType, SIGNAL(changed(int)), this, SLOT(onSelMultiArrayTypeChanged(int)));
    conf["selMultiArrayType"]=selMultiArrayType;
    slayout->addWidget(selMultiArrayType);
    multiarrayN=new val_selector(1, "N of runs:", 1, 10000, 0);
    connect(multiarrayN, SIGNAL(changed(double)), this, SLOT(onMultiarrayNChanged(double)));
    conf["multiarrayN"]=multiarrayN;
    slayout->addWidget(multiarrayN);
    selArrayFocusBlur=new val_selector(2, "Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
    conf["selArrayFocusBlur"]=selArrayFocusBlur;
    selArrayFocusBlur->setVisible(false);
    slayout->addWidget(selArrayFocusBlur);
    selArrayFocusThresh=new val_selector(0.2, "2nd Derivative Exclusion Threshold: ", 0, 1, 4);
    conf["selArrayFocusThresh"]=selArrayFocusThresh;
    selArrayFocusThresh->setToolTip("Try out values in Depth Eval.");
    selArrayFocusThresh->setVisible(false);
    slayout->addWidget(selArrayFocusThresh);

    slayout->addWidget(new hline);
    slayout->addWidget(new QLabel("PROCESSING:"));
    slayout->addWidget(new QLabel("Crop individual points (for shifted beams)(setting not saved):"));
    cropTop =new QSpinBox;  cropTop->setRange(0,100);  cropTop->setPrefix("Top: ");  cropTop->setSuffix(" px");
    cropBttm=new QSpinBox; cropBttm->setRange(0,100); cropBttm->setPrefix("Bottom: "); cropBttm->setSuffix(" px");
    cropLeft=new QSpinBox; cropLeft->setRange(0,100); cropLeft->setPrefix("Left: "); cropLeft->setSuffix(" px");
    cropRght=new QSpinBox; cropRght->setRange(0,100); cropRght->setPrefix("Right: "); cropRght->setSuffix(" px");
    slayout->addWidget(new twid(cropTop,cropBttm));
    slayout->addWidget(new twid(cropLeft,cropRght));


    gui_processing=new QWidget;
    playout=new QVBoxLayout;
    gui_processing->setLayout(playout);
    btnProcessFocusMes=new QPushButton("Select Folders to Process Focus Measurements (Array method)");
    connect(btnProcessFocusMes, SIGNAL(released()), this, SLOT(onProcessFocusMes()));
    playout->addWidget(new twid(btnProcessFocusMes));
}
void pgCalib::onMultiarrayNChanged(double val){
    selArrayFocusBlur->setVisible(val!=1);
    selArrayFocusThresh->setVisible(val!=1);
    selArray(selArrayType->index, selMultiArrayType->index);
}
void pgCalib::onSelArrayTypeChanged(int index){
    selArray(index, selMultiArrayType->index);
}
void pgCalib::onSelMultiArrayTypeChanged(int index){
    selArray(selArrayType->index, index);
}
void pgCalib::onPrerunTypeChanged(int type){
    selPeakXshift->setVisible(type==1);
    selPeakYshift->setVisible(type==1);
}
void pgCalib::selArray(int ArrayIndex, int MultiArrayIndex){
    if(multiarrayN->val==1) MultiArrayIndex=0;
    bool foc=ArrayIndex==1 || ArrayIndex==2 || ArrayIndex==4 || MultiArrayIndex==1;
    bool dur=ArrayIndex==0 || ArrayIndex==2 || ArrayIndex==3 || MultiArrayIndex==2;
    bool pla=MultiArrayIndex==3;
    selArrayFocA->setLabel(foc?"Focus lower limit":"Focus");
    selArrayDurA->setLabel(dur?"Duration lower limit":"Duration");
    selPlateauA->setLabel(pla?"Height lower limit":"Height");
    selArrayFocB->setVisible(foc);
    selArrayDurB->setVisible(dur);
    selPlateauB->setVisible(pla);
}
bool pgCalib::goToNearestFree(double radDilat, double radRandSpread, double blur, double thrs, double radDilaty, bool convpx2um){
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //if there is a measurement in progress, wait till its done

    // first check if the current scan result is valid (done on current position)
    double tmp[3];
    tmp[0]=go.pRPTY->getMotionSetting("X",CTRL::mst_position);
    tmp[1]=go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
    tmp[2]=go.pRPTY->getMotionSetting("Z",CTRL::mst_position);
    bool redoScan=false;
    const pgScanGUI::scanRes* res=scanRes->get();
    if(res!=nullptr){
        for(int i=0;i!=3;i++) if(res->pos[i]!=tmp[i])redoScan=true;
    }else redoScan=true;
    if(redoScan){
        pgSGUI->doOneRound({0,0,-1,0}); // forces full ROI
        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        res=scanRes->get();
    }
    int dil=(convpx2um?radDilat:pgMGUI->mm2px(radDilat/1000)-0.5); if(dil<0) dil=0;
    int dily=(convpx2um?radDilaty:pgMGUI->mm2px(radDilaty/1000)-0.5); if(dily<0) dily=0;
    cv::Mat mask=pgDpEv->getMaskFlatness(res, dil, thrs, blur, dily);
    int ptX,ptY;
    imgAux::getNearestFreePointToCenter(&mask, pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX), pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY), ptX, ptY, convpx2um?radRandSpread:pgMGUI->mm2px(radRandSpread/1000));
    if(ptX==-1){
        QMessageBox::warning(this, "Warning", "No free nearby!");
        delete scanRes;
        return true;
    }

    pgMGUI->move(pgMGUI->px2mm(ptX-res->depth.cols/2),-pgMGUI->px2mm(ptY-res->depth.rows/2),0);
    delete scanRes;
    return false;
}

void pgCalib::onWCF(){
    if(!btnWriteCalib->isChecked()) return;
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}

    std::time_t time=std::time(nullptr); std::tm ltime=*std::localtime(&time);
    std::stringstream ifolder;
    ifolder<<lastFolder;
    ifolder<<std::put_time(&ltime, "%Y-%m-%d-%H-%M-%S");
    std::string saveFolderName=QFileDialog::getSaveFileName(this, tr("Select Folder for Saving Calibration Data"),QString::fromStdString(ifolder.str()),tr("Folders")).toStdString();
    if(saveFolderName.empty()){
        btnWriteCalib->setChecked(false);
        return;
    }
    lastFolder=saveFolderName.substr(0,saveFolderName.find_last_of("/"));
    saveFolderName+="/";
    lastFolder+="/";
    cv::utils::fs::createDirectory(saveFolderName);

    WCFArray(saveFolderName);
}

void pgCalib::saveMainConf(std::string filename){
    std::ofstream setFile(filename);     //this file contains some settings:
    setFile <<"Objective_displacement_X(mm) Objective_displacement_Y(mm) Objective_displacement_Z(mm) MirauXYmmppx(mm/px)\n";
    setFile << std::fixed << std::setprecision(6);
    setFile <<pgMGUI->objectiveDisplacementX<<" "<<pgMGUI->objectiveDisplacementY<<" "<<pgMGUI->objectiveDisplacementZ<<" ";
    setFile << std::defaultfloat <<pgMGUI->getNmPPx()/1000000<<"\n";
    setFile.close();
}
void pgCalib::saveConf(std::string filename, double duration, double focus, double plateau, double peak, double peakXshift, double peakYshift){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<"Duration(ms) Focus(um)";
    if(peak!=0 || plateau!=0) setFile <<" Plateau(nm)";
    if(peak!=0) setFile<<" Peak(nm) PeakXShift(um) PeakYShift(um)";
    setFile <<"\n"<<duration<<" "<<focus;
    if(peak!=0 || plateau!=0) setFile <<" "<<plateau;
    if(peak!=0) setFile<<" "<<peak<<" "<<peakXshift<<" "<<peakYshift;
    setFile <<"\n";
    setFile.close();
}

void pgCalib::WCFArray(std::string folder){
    cv::Mat WArray(selArrayYsize->val,selArrayXsize->val,CV_64FC2,cv::Scalar(0,0));         //Duration(ms), Focus(um)
    int arraySizeDur{1}, arraySizeFoc{1}, arraySizePla{1};
    int index=selArrayType->index;
    int mindex=selMultiArrayType->index;
    bool multiFoc=false;
    bool multiDur=false;
    switch(index){
    case 0: arraySizeDur=selArrayYsize->val*selArrayXsize->val;     //Duration (XY), no repeat
            break;
    case 1: arraySizeFoc=selArrayYsize->val*selArrayXsize->val;     //Focus (XY), no repeat
            break;
    case 2: arraySizeDur=selArrayXsize->val;                        //Duration-Focus (X,Y)
            arraySizeFoc=selArrayYsize->val;
            break;
    case 3: arraySizeDur=selArrayXsize->val;                        //Duration (XY), repeat
            break;
    case 4: arraySizeFoc=selArrayXsize->val;                        //Focus (XY), repeat
            break;
    }
    switch(mindex){
    case 1: arraySizeFoc*=multiarrayN->val;                         // Focus
            multiFoc=true;
            break;
    case 2: arraySizeDur*=multiarrayN->val;                         // Duration
            multiDur=true;
            break;
    case 3: arraySizePla=multiarrayN->val;                          // Plateau
            break;
    }

    std::vector<double> arrayDur; arrayDur.reserve(arraySizeDur);
    std::vector<double> arrayFoc; arrayFoc.reserve(arraySizeFoc);
    std::vector<double> arrayPla; arrayFoc.reserve(arraySizePla);
    if(arraySizeDur==1) arrayDur.push_back(selArrayDurA->val);
    else for(int i=0;i!=arraySizeDur; i++) arrayDur.push_back(selArrayDurA->val*(1-(double)i/(arraySizeDur-1))+selArrayDurB->val*((double)i/(arraySizeDur-1)));
    if(arraySizeFoc==1) arrayFoc.push_back(selArrayFocA->val);
    else for(int i=0;i!=arraySizeFoc; i++) arrayFoc.push_back(selArrayFocA->val*(1-(double)i/(arraySizeFoc-1))+selArrayFocB->val*((double)i/(arraySizeFoc-1)));
    if(arraySizePla==1) arrayPla.push_back(selPlateauA->val);
    else for(int i=0;i!=arraySizePla; i++) arrayPla.push_back(selPlateauA->val*(1-(double)i/(arraySizePla-1))+selPlateauB->val*((double)i/(arraySizePla-1)));

    if(selArrayRandomize->val){                         //randomize if chosen
        std::mt19937 rnd(std::random_device{}());
        std::shuffle(arrayDur.begin(), arrayDur.end(), rnd);
        std::shuffle(arrayFoc.begin(), arrayFoc.end(), rnd);
        std::shuffle(arrayPla.begin(), arrayPla.end(), rnd);
    }


    double xSize=WArray.cols*pgMGUI->mm2px(selArraySpacing->val/1000);
    double ySize=WArray.rows*pgMGUI->mm2px(selArraySpacing->val/1000);

    // set ROI
    bool err=false;
    cv::Rect ROI, sROI;
    sROI.width=pgMGUI->mm2px(selArraySpacing->val/1000);
    sROI.height=sROI.width;

    if(go.pGCAM->iuScope->camCols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX)<0 || go.pGCAM->iuScope->camCols/2-xSize/2-pgBeAn->writeBeamCenterOfsX+xSize>=go.pGCAM->iuScope->camCols){
        err=true;
    }else{
        ROI.x=go.pGCAM->iuScope->camCols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX);
        ROI.width=xSize;
    }
    if(go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY)<0 || go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY)+ySize>=go.pGCAM->iuScope->camRows){
        err=true;
    }else{
        ROI.y=go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY);
        ROI.height=ySize;
    }
    if(err){
        QMessageBox::critical(gui_activation, "Error", "The calibration ROI does not fit the viewport, aborting calibration.\n");
        btnWriteCalib->setChecked(false);
        return;
    }

    saveMainConf(util::toString(folder,"/main-settings.txt"));
    bool isPlateau=selPrerunType->index==0;
    if(multiarrayN->val>1){
        std::ofstream wfile(util::toString(folder,isPlateau?"/Plateau.txt":"/Peak.txt"));
        for(int i=0;i!=arrayPla.size(); i++){
            wfile<<arrayPla[i]<<"\n";
        }wfile.close();
    }
    for(int n=0;n!=multiarrayN->val;n++){
        for(int i=0;i!=WArray.cols; i++) for(int j=0;j!=WArray.rows; j++){              //populate 3D x2 array
            if(index==0) WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[i+j*WArray.cols+(multiDur?n:0)*WArray.rows*WArray.cols];
            else if(index==2 || index==3) WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[i+(multiDur?n:0)*WArray.cols];
            else WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[multiDur?n:0];
            if(index==1) WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[i+j*WArray.cols+(multiFoc?n:0)*WArray.rows*WArray.cols];
            else if(index==2)  WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[j+(multiFoc?n:0)*WArray.rows];
            else if(index==4)  WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[i+(multiFoc?n:0)*WArray.cols];
            else WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[multiFoc?n:0];
        }
        if(transposeMat->val)
            cv::transpose(WArray,WArray);

        if(n!=0)
            if(!btnWriteCalib->isChecked() || goToNearestFree(WArray.cols*selArraySpacing->val, selArraySpacing->val, selArrayFocusBlur->val, selArrayFocusThresh->val, WArray.rows*selArraySpacing->val)){
                QMessageBox::critical(this, "Error", "Calibration aborted. Measurements up to this point were saved.");
                btnWriteCalib->setChecked(false);
                return;
            }

        if(saveMats->val){      //export values as matrices, for convenience
            std::string names[2]={"Duration","Focus"};
            for(int k=0;k!=2;k++){
                std::ofstream wfile(util::toString(folder,"/", names[k],".txt"), std::ofstream::out | std::ofstream::app);
                for(int i=0;i!=WArray.cols; i++){
                    for(int j=0;j!=WArray.rows; j++)
                        wfile<<WArray.at<cv::Vec2d>(j,i)[k]<<" ";
                    wfile<<"\n";
                }
                wfile<<"\n";
                wfile.close();
            }
        }
        WCFArrayOne(WArray, arrayPla.size()==1?arrayPla[0]:arrayPla[n], ROI, sROI, folder, static_cast<double>(n)/multiarrayN->val, isPlateau, selPeakXshift->val, selPeakYshift->val);
        if(transposeMat->val)
            cv::transpose(WArray,WArray);
    }
    btnWriteCalib->setChecked(false);
}
void pgCalib::WCFArrayOne(cv::Mat WArray, double plateau, cv::Rect ROI, cv::Rect sROI, std::string folder, double progressfac, bool isPlateau, double peakXshift, double peakYshift){
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    double xOfs=((WArray.cols-1)*selArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selArraySpacing->val)/2000;

    pgFGUI->doRefocus(true, ROI);

    if(plateau!=0){
        if(isPlateau){
            cv::Mat mplateau(WArray.rows+1, WArray.cols+1, CV_32F, cv::Scalar(plateau/1000000));
            pgWr->onWriteDM(&mplateau, 0, selArraySpacing->val/1000);
        }else{
            cv::Mat mplateau(WArray.rows, WArray.cols, CV_32F, cv::Scalar(plateau/1000000));
            pgMGUI->move(peakXshift/1000,peakYshift/1000,0);
            pgWr->onWriteDM(&mplateau, 0, selArraySpacing->val/1000, selArraySpacing->val/1000);
            pgMGUI->move(-peakXshift/1000,-peakYshift/1000,0);
        }
    }

    const pgScanGUI::scanRes* res;
    pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, discardMaskRoiThresh, maxRedoScanTries);

    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/before"), false, true, savePic->val?1:0);


    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        cv::utils::fs::createDirectory(util::toString(folder,"/",i+j*WArray.cols));
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",i+j*WArray.cols,"/before"), true, savePic->val?1:0);
    }

    {std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_proc);
        pgMGUI->chooseObj(false);
        pgMGUI->move(-xOfs,-yOfs,0);
        pgSGUI->MLP.progress_proc=100*progressfac;
        CTRL::CO CO(go.pRPTY);
        CO.clear(true);
        for(int j=0;j!=WArray.rows; j++){
            for(int i=0;i!=WArray.cols; i++){
                if(!btnWriteCalib->isChecked()){   //abort
                    QMessageBox::critical(gui_activation, "Error", "Calibration aborted. Measurements up to this point were saved.\n");
                    delete scanRes;
                    return;
                }

                pgMGUI->corCOMove(CO,0,0,WArray.at<cv::Vec2d>(j,i)[1]/1000);
                CO.addHold("X",CTRL::he_motion_ontarget);
                CO.addHold("Y",CTRL::he_motion_ontarget);
                CO.addHold("Z",CTRL::he_motion_ontarget);
                CO.pulseGPIO("wrLaser",WArray.at<cv::Vec2d>(j,i)[0]/1000);
                pgMGUI->corCOMove(CO,0,0,-WArray.at<cv::Vec2d>(j,i)[1]/1000);
                saveConf(util::toString(folder,"/",i+j*WArray.cols,"/settings.txt"), WArray.at<cv::Vec2d>(j,i)[0], WArray.at<cv::Vec2d>(j,i)[1], isPlateau?plateau:0, isPlateau?0:plateau, isPlateau?0:static_cast<double>(selArraySpacing->val), isPlateau?0:static_cast<double>(selArraySpacing->val));

                CO.execute();
                CO.clear(true);

                while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
                if(i!=WArray.cols-1) pgMGUI->corCOMove(CO,selArraySpacing->val/1000,0,0);
            }
            if(j!=WArray.rows-1) pgMGUI->corCOMove(CO,-2*xOfs,selArraySpacing->val/1000,0);
        }
        pgMGUI->move(-xOfs,-yOfs,0);
        pgMGUI->chooseObj(true);
    }

    pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, discardMaskRoiThresh, maxRedoScanTries,0,savePic->val?1:0);
    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/after"), false, true, savePic->val?1:0);

    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",i+j*WArray.cols,"/after"), true, savePic->val?1:0);
    }

    delete scanRes;
}

typedef dlib::matrix<double,2,1> input_vector;
typedef dlib::matrix<double,7,1> parameter_vector;
double gaussResidual(const std::pair<input_vector, double>& data, const parameter_vector& params){
    double x0=params(0);  double x=data.first(0);
    double y0=params(1);  double y=data.first(1);
    double a=params(2);
    double wx=params(3);
    double wy=params(4);
    double an=params(5);
    double i0=params(6);
    //double model=i0+a*exp(-(pow(x-x0,2)+pow(y-y0,2))/2/pow(w,2));
    //double model=i0+a*exp(-(  pow(x-x0,2)/2*(cos(an)/pow(wx,2)+sin(an)/pow(wy,2)) +  pow(y-y0,2)/2*(cos(an)/pow(wy,2)+sin(an)/pow(wx,2)) ));  //seems to be wrong
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    double model=i0+a*exp(-A*pow(x-x0,2)-B*(x-x0)*(y-y0)-C*pow(y-y0,2));
    //if(i0<0 || x0<0 || y0<0 || a<0 || a>255) return 9999;
    return model-data.second;
}
bool folderSort(std::string i,std::string j){
    size_t posi=i.find_last_of("/");
    size_t posj=j.find_last_of("/");
    return (std::stoi(i.substr(posi+1,9))<std::stoi(j.substr(posj+1,9)));
}
void pgCalib::onProcessFocusMes(){
    std::vector<std::string> readFolders;   //folders still yet to be checked
    std::vector<std::string> measFolders;   //folders that contain the expected measurement files

    readFolders.emplace_back(QFileDialog::getExistingDirectory(this, "Select Folder Contatining Measurements. It will be Searched Recursively.", QString::fromStdString(lastFolder)).toStdString());
    if(readFolders.back().empty()) return;
    lastFolder=readFolders.back().substr(0,readFolders.back().find_last_of("/"));
    std::string saveName=readFolders.back()+"/proc.txt";
    DIR *wp;
    struct dirent *entry;
    struct stat filetype;
    std::string curFile;
    std::string curFolder;
    bool dirHasMes[5]{false,false,false,false};
    while(!readFolders.empty()){
        curFolder=readFolders.back();
        readFolders.pop_back();
        wp=opendir(curFolder.c_str());
        if(wp!=nullptr) while(entry=readdir(wp)){
            curFile=entry->d_name;
            if (curFile!="." && curFile!=".."){
                curFile=curFolder+'/'+entry->d_name;

                stat(curFile.data(),&filetype);
                if(filetype.st_mode&S_IFDIR) readFolders.push_back(curFile);
                else if (filetype.st_mode&S_IFREG){
                    if(strcmp(entry->d_name,"settings.txt")==0) dirHasMes[0]=true;
                    else if(strcmp(entry->d_name,"before.pfm")==0) dirHasMes[1]=true;
                    else if(strcmp(entry->d_name,"after.pfm")==0) dirHasMes[2]=true;
                    else if(strcmp(entry->d_name,"after-RF.png")==0) dirHasMes[3]=true;       //nonessential for backward compatibility
                }
            }
        }
        closedir(wp);
        if(dirHasMes[0]&dirHasMes[1]&dirHasMes[2]) measFolders.push_back(curFolder);
    }
    std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_comp);
    pgSGUI->MLP.progress_comp=0;

    //sort folders by number
    std::sort(measFolders.begin(), measFolders.end(), folderSort);

    std::ofstream wfile(saveName);
    int n=0;
    wfile<<"# 1: valid measurement(=1)\n";
    wfile<<"# 2: focus distance(um)\n";
    wfile<<"# 3: peak height(nm)\n";
    wfile<<"# 4: X width (1/e^2)(um)\n";
    wfile<<"# 5: Y width (1/e^2)(um)\n";
    wfile<<"# 6: ellipse angle(rad)\n";
    wfile<<"# 7: XY width (1/e^2)(um)\n";
    wfile<<"# 8: X offset(um)\n";
    wfile<<"# 9: Y offset(um)\n";
    wfile<<"# 10: XY offset(um)\n";
    wfile<<"# 11: plateau(nm)\n";
    wfile<<"# 12: duration(ms)\n";
    wfile<<"# 13: MeanAbs.ReflectivityDeriv.(a.u.)\n";
    wfile<<"# 14: Max absolute 1st der. (nm/um)\n";
    wfile<<"# 15: Min Laplacian (nm/um^2)\n";
    wfile<<"# 16: Max Laplacian (nm/um^2)\n";
    wfile<<"# 17: peak height(nm)(max)\n";
    wfile<<"# 18: peak height(nm)(max-min)\n";
    wfile<<"# 19: X width (FWHM)(um)\n";
    wfile<<"# 20: Y width (FWHM)(um)\n";
    wfile<<"# 21: XY width (FWHM)(um)\n";
    wfile<<"# 22: prepeak(nm)\n";
    wfile<<"# 23: prepeakXoffs(um)\n";
    wfile<<"# 24: prepeakYoffs(um)\n";

    for(auto& fldr:measFolders){ n++;
        double focus;
        double duration;
        double plateau{0};
        double prepeak{0}, prepeakXofs{0}, prepeakYofs{0};
        std::ifstream ifs(util::toString(fldr,"/settings.txt"));
        ifs.ignore(std::numeric_limits<std::streamsize>::max(),'\n');       // ignore header
        ifs>>duration;
        ifs>>focus;
        ifs>>plateau;
        ifs>>prepeak;
        ifs>>prepeakXofs;
        ifs>>prepeakYofs;
        ifs.close();

        pgScanGUI::scanRes scanBefore, scanAfter;
        if(!pgScanGUI::loadScan(&scanBefore, util::toString(fldr,"/before.pfm"))) continue;
        if(!pgScanGUI::loadScan(&scanAfter, util::toString(fldr,"/after.pfm"))) continue;
        pgScanGUI::scanRes scanDif=pgSGUI->difScans(&scanBefore, &scanAfter);
        if(cropTop->value()!=0 || cropBttm->value()!=0 || cropLeft->value()!=0 || cropRght->value()!=0){
            if(cropTop->value()+cropBttm->value()>=scanDif.depth.rows || cropRght->value()+cropLeft->value()>=scanDif.depth.cols) {std::cerr<<"Cropped dimensions are larger than scan sizes. Aborting processing.\n"; return;}
            scanDif.mask =scanDif.mask (cv::Rect(cropLeft->value(), cropTop->value(), scanDif.depth.cols-cropLeft->value()-cropRght->value(), scanDif.depth.rows-cropTop->value()-cropBttm->value()));
            scanDif.depth=scanDif.depth(cv::Rect(cropLeft->value(), cropTop->value(), scanDif.depth.cols-cropLeft->value()-cropRght->value(), scanDif.depth.rows-cropTop->value()-cropBttm->value()));
            scanDif.maskN=scanDif.maskN(cv::Rect(cropLeft->value(), cropTop->value(), scanDif.depth.cols-cropLeft->value()-cropRght->value(), scanDif.depth.rows-cropTop->value()-cropBttm->value()));
        }
        pgScanGUI::saveScan(&scanDif, util::toString(fldr,"/scandif.pfm"));

//        cv::Mat rescaleDepth, rescaleMask;                                    //this can be done to increase performance
//        cv::resize(scanDif.depth,rescaleDepth,cv::Size(),0.2,0.2);            //dont forget to rescale the measured widths
//        cv::resize(scanDif.mask,rescaleMask,cv::Size(),0.2,0.2);

        std::vector<std::pair<input_vector, double>> data;
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
            if(scanDif.mask.at<uchar>(j,i)==0) data.push_back(std::make_pair(input_vector{(double)i,(double)j},scanDif.depth.at<float>(j,i)));

        parameter_vector res{(double)scanDif.depth.cols/2,(double)scanDif.depth.rows/2,scanDif.max-scanDif.min,(double)scanDif.depth.rows, (double)scanDif.depth.rows, 0.01, scanDif.min};

        dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7,100), gaussResidual, derivative(gaussResidual), data, res);
        while(res(5)>M_PI) res(5)-=M_PI;
        while(res(5)<0) res(5)+=M_PI;
        if(res(5)>=M_PI/2){
           res(5)-= M_PI/2;
           double tmp=res(3);
           res(3)=res(4);
           res(4)=tmp;
        }

        double intReflDeriv{-1};
        if(dirHasMes[4]){
            cv::Mat refl=imread(util::toString(fldr,"/after-RF.png"), cv::IMREAD_GRAYSCALE);
            if(refl.empty()) refl=imread(util::toString(fldr,"/pic.png"), cv::IMREAD_GRAYSCALE);         // pic.png for backward compatibility
            cv::Mat reflS;
            cv::Mat derv, dervy;
            cv::bilateralFilter(refl, reflS, -1, 4, 4);  //smooth it a bit
            cv::Sobel(reflS, derv, CV_32F,1,0);          //first derivative
            cv::Sobel(reflS, dervy, CV_32F,0,1);
            cv::add(derv, dervy, derv);
            intReflDeriv=cv::mean(cv::abs(derv))[0];
        }

        //find max abs first derivative of depth (nm/um):
        cv::Mat com, derv, dervy;
        cv::bilateralFilter(scanDif.depth, com, -1, 3, 3);  //smooth it a bit
        cv::Sobel(com, derv, CV_32F,1,0);                   //first derivative
        cv::Sobel(com, dervy, CV_32F,0,1);                  //first derivative
        double maxDepthDer;
        cv::add(derv, dervy, derv);
        cv::minMaxIdx(derv, nullptr, &maxDepthDer);
        //find min, max laplacian of depth (nm/um^2):
        cv::Laplacian(com, derv, CV_32F);
        double minDepthLaplacian, maxDepthLaplacian;
        cv::minMaxIdx(derv, &minDepthLaplacian, &maxDepthLaplacian);

        double XYumppx=scanDif.XYnmppx/1000;
        maxDepthDer/=XYumppx;
        minDepthLaplacian/=XYumppx;
        maxDepthLaplacian/=XYumppx;

        double Xwidth=2*abs(res(3))*XYumppx;
        double Ywidth=2*abs(res(4))*XYumppx;
        double XYwidth=(Xwidth+Ywidth)/2;
        double Xofs=res(0)*XYumppx;
        double Yofs=res(1)*XYumppx;
        double XYofs=sqrt(pow(Xofs,2)+pow(Yofs,2));
        const double toFWHM=sqrt(2*log(2));

        int valid=1;
        if(res(0)<0 || res(0)>scanDif.depth.cols || res(1)<0 || res(1)>scanDif.depth.rows || res(2)<=0) valid=0;    //center of the fit is out of frame or other things that indicate fit faliure
        wfile<<valid<<" ";                      // 1: valid measurement(=1)
        wfile<<focus<<" ";                      // 2: focus distance(um)
        wfile<<res(2)<<" ";                     // 3: peak height(nm)
        wfile<<Xwidth<<" ";                     // 4: X width (1/e^2)(um)
        wfile<<Ywidth<<" ";                     // 5: Y width (1/e^2)(um)
        wfile<<res(5)<<" ";                     // 6: ellipse angle(rad)
        wfile<<XYwidth<<" ";                    // 7: XY width (1/e^2)(um)
        wfile<<Xofs<<" ";                       // 8: X offset(um)
        wfile<<Yofs<<" ";                       // 9: Y offset(um)
        wfile<<XYofs<<" ";                      // 10: XY offset(um)
        wfile<<plateau<<" ";                    // 11: plateau(nm)
        wfile<<duration<<" ";                   // 12: duration(ms)
        wfile<<intReflDeriv<<" ";               // 13: MeanAbs.ReflectivityDeriv.(a.u.)
        wfile<<maxDepthDer<<" ";                // 14: Max absolute 1st der. (nm/um)
        wfile<<minDepthLaplacian<<" ";          // 15: Min Laplacian (nm/um^2)
        wfile<<maxDepthLaplacian<<" ";          // 16: Max Laplacian (nm/um^2)
        wfile<<scanDif.max<<" ";                // 17: peak height(nm)(max)
        wfile<<scanDif.max-scanDif.min<<" ";    // 18: peak height(nm)(max-min)
        wfile<<Xwidth*toFWHM<<" ";              // 19: X width (FWHM)(um)
        wfile<<Ywidth*toFWHM<<" ";              // 20: Y width (FWHM)(um)
        wfile<<XYwidth*toFWHM<<" ";             // 21: XY width (FWHM)(um)
        wfile<<prepeak<<" ";                    // 22: prepeak(nm)
        wfile<<prepeakXofs<<" ";                // 23: prepeakXoffs(um)
        wfile<<prepeakYofs<<"\n";               // 24: prepeakYoffs(um)
        pgSGUI->MLP.progress_comp=100./measFolders.size()*n;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
    }
    wfile.close();
    pgSGUI->MLP.progress_comp=100;
}

void pgCalib::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status;
}
void pgCalib::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double xSize;
    double ySize;
    xSize=pgMGUI->mm2px(selArrayXsize->val*selArraySpacing->val/1000);
    ySize=pgMGUI->mm2px(selArrayYsize->val*selArraySpacing->val/1000);
    if(transposeMat->val) std::swap(xSize,ySize);
    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX), img->rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY), xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
