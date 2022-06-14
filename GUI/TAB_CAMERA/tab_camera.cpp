#include "tab_camera.h"
#include "GUI/mainwindow.h"
#include "includes.h"
#include <filesystem>

tab_camera::tab_camera(QWidget* parent){
    pgScanGUI::parent=this; //for dialogs in pgScanGUI static functions
    conf["exclColor-B"]=exclColor.val[0];
    conf["exclColor-G"]=exclColor.val[1];
    conf["exclColor-R"]=exclColor.val[2];

    layout=new QHBoxLayout;
    parent->setLayout(layout);

    LDisplay=new iImageDisplay;
    LDisplay->parent=this;
    LDisplay->setMouseTracking(false);
    LDisplay->setScaledContents(false);
    LDisplay->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    LDisplay->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    tBarW=new QWidget;
    layoutTBarW= new QVBoxLayout;
    tBarW->setLayout(layoutTBarW);
    tBarW->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);

    layout->addWidget(LDisplay);
    layout->addWidget(tBarW);

    selDisp=new smp_selector("Display mode:", 0, {"Camera","Depth Map","Depth Map SD","Reflectivity"});
    selObjective=new smp_selector("Objective:", 0, {"Mirau","Writing"});
    layoutTBarW->addWidget(new twid(selDisp,new QLabel("   |   "),selObjective));

    TWCtrl=new QTabWidget;
    TWCtrl->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
    layoutTBarW->addWidget(TWCtrl);

    pgSGUI=new pgScanGUI(MLP, sROI);
    conf["pgScan"]=pgSGUI->conf;
    scanRes=pgSGUI->result.getClient();
    pgMGUI=new pgMoveGUI(selObjective, loadedScan.pos);
    conf["pgMove"]=pgMGUI->conf;
    pgTGUI=new pgTiltGUI;
    conf["pgTilt"]=pgTGUI->conf;
    pgFGUI=new pgFocusGUI(MLP, sROI, pgSGUI, pgMGUI);
    conf["pgFocus"]=pgFGUI->conf;
    pgPRGUI=new pgPosRepGUI(pgMGUI);
    cm_sel=new smp_selector("Select colormap: ", 0, OCV_CM::qslabels());
    conf["cm_sel"]=cm_sel;
    showAbsHeight=new checkbox_gs(false,"Abs. height.");
    conf["showAbsHeight"]=showAbsHeight;
    connect(showAbsHeight, SIGNAL(changed()), this, SLOT(onShowAbsHeightChanged()));
    cMap=new colorMap(cm_sel, exclColor, showAbsHeight, pgSGUI, pgTGUI);
    conf["colormap"]=cMap->conf;
    tCG=new tabCamGnuplot(showAbsHeight);
    conf["tabCamGnuplot"]=tCG->conf;
    pgSGUI->pgMGUI=pgMGUI;
    pgCor=new pgCorrection(pgSGUI, pgMGUI);
    conf["pgCorrection"]=pgCor->conf;
    pgSGUI->useCorr=&pgCor->useCorr;
    pgSGUI->cor=&pgCor->cor;
    connect(pgCor, SIGNAL(sendToDisplay(pgScanGUI::scanRes)), this, SLOT(showScan(pgScanGUI::scanRes)));
    connect(pgSGUI->xDifShift, SIGNAL(changed()), this, SLOT(diff2RawCompute()));
    connect(pgSGUI->yDifShift, SIGNAL(changed()), this, SLOT(diff2RawCompute()));
    camSet=new cameraSett(pgSGUI->getExpMinMax, pgMGUI); connect(pgSGUI, SIGNAL(doneExpMinmax(int,int)), camSet, SLOT(doneExpMinmax(int,int)));
    conf["camera_settings"]=camSet->conf;

    pgBeAn=new pgBeamAnalysis(pgMGUI);
    conf["pgBeamAnalysis"]=pgBeAn->conf;
    ovl.emplace_back();
    pgWrt=new pgWrite(pgBeAn, pgMGUI, MLP, pgSGUI, ovl.back(), pgFGUI);
    conf["pgWrite"]=pgWrt->conf;
    ovl.emplace_back();
    pgCal=new pgCalib(pgSGUI, pgFGUI, pgMGUI, pgBeAn, pgWrt, ovl.back());
    conf["pgCalib"]=pgCal->conf;

    addInfo=new QLabel; addInfo->setMargin(10);
    addInfo->setVisible(false);

    pageMotion=new twd_selector;
        pageMotion->addWidget(addInfo);
        pageMotion->addWidget(pgSGUI->gui_activation);
        pageMotion->addWidget(pgMGUI->gui_activation);
        pageMotion->addWidget(pgTGUI->gui_activation);
        pageMotion->addWidget(pgFGUI->gui_activation);
        pageMotion->addWidget(pgPRGUI);

    pageWriting=new twd_selector;
        pageWriting->addWidget(pgWrt->gui_activation);\

    pageProcessing=new twd_selector;
        loadRawBtn=new QPushButton("Load measurement"); connect(loadRawBtn, SIGNAL(released()), this, SLOT(onLoadDepthMapRaw()));
        diff2RawBtn=new QPushButton("Load 2 measurements and dif them"); connect(diff2RawBtn, SIGNAL(released()), this, SLOT(onDiff2Raw()));
        avgNRawBtn=new QPushButton("Average all measurements in folder"); connect(avgNRawBtn, SIGNAL(released()), this, SLOT(onAvgNRawBtn()));
        avgNRawBtn->setToolTip("Averages files in subfolders as well.");
        avgNRawMarginCut=new val_selector(0, "Cut: ", 0, 1000, 0);
        avgNRawMarginCut->setToolTip("Cut margin: all scans loaded for averaging will have this ammount of pixels removed from all margins.");
        avgNRawMarginRef=new val_selector(0, "Ref: ", 0, 1000, 0);
        avgNRawMarginRef->setToolTip("Reference margin: the averaging will use pixels within this margin as a background reference. Set to 0 to use whole image.");
        combineMes=new QPushButton("Combine measurements"); connect(combineMes, SIGNAL(released()), this, SLOT(onCombineMes()));
        combineUseRefl=new QCheckBox("Use Reflectivity");

        pageProcessing->addWidget(new vtwid(new twid(loadRawBtn),new twid(diff2RawBtn),new twid(avgNRawBtn,avgNRawMarginCut,avgNRawMarginRef),new twid(combineMes,combineUseRefl),pgSGUI->gui_processing,false, false));

    pageSettings=new twd_selector("","Select");
    connect(pageSettings, SIGNAL(changed(int)), this, SLOT(on_tab_change(int)));
        pageSettings->addWidget(pgSGUI->gui_settings,"Scan");   index_pgSGUI=0;
        pageSettings->addWidget(pgMGUI->gui_settings,"Move");
        pageSettings->addWidget(pgTGUI->gui_settings,"Tilt");
        pageSettings->addWidget(pgFGUI->gui_settings,"Focus");  index_pgFGUI=3;
        pageSettings->addWidget(pgCor->gui_settings,"Scan Correction");
        pageSettings->addWidget(cMap,"ColorMap");
        pageSettings->addWidget(camSet,"Camera");               index_camSet=6;
        pageSettings->addWidget(pgCal->gui_settings,"Write Calibration");
        pageSettings->addWidget(pgBeAn->gui_settings,"Beam Centering");
        pageSettings->addWidget(tCG,"Gnuplot");
        pageSettings->addWidget(pgWrt->gui_settings, "Writing");

    TWCtrl->addTab(pageMotion,"Motion");
    TWCtrl->addTab(pageWriting,"Writing");
    TWCtrl->addTab(pageProcessing,"Processing");
    TWCtrl->addTab(pageSettings,"Settings");

    layoutTBarW->addWidget(cm_sel);
    epc_sel=new QPushButton("Excl."); epc_sel->setFlat(true); epc_sel->setIcon(QPixmap(":/color.svg"));
    connect(epc_sel, SIGNAL(released()), this, SLOT(on_EP_sel_released()));
    cm_sel->addWidget(epc_sel);
    cm_sel->addWidget(showAbsHeight);
    pgHistGUI=new pgHistogrameGUI(400, 50, cm_sel, exclColor);
    conf["pgHistograme"]=pgHistGUI->conf;
    layoutTBarW->addWidget(pgHistGUI);

    main_show_scale=new checkbox_gs(false,"ScaleBar");
    conf["main_show_scale"]=main_show_scale;
    main_show_target=new checkbox_gs(false,"Target");
    conf["main_show_target"]=main_show_target;
    main_CLAHE_writing=new checkbox_gs(false,"CLAHE(WrObj)");
    conf["main_CLAHE_writing"]=main_CLAHE_writing;
    layoutTBarW->addWidget(new twid(main_show_scale, main_show_target, main_CLAHE_writing));

    measPB=new QProgressBar; measPB->setRange(0,100);
    compPB=new QProgressBar; compPB->setRange(0,100);
    layoutTBarW->addWidget(new twid(new QLabel("Action: "), measPB, new QLabel("Computation: "), compPB));

    timer=new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));

    clickMenu=new QMenu;
    clickMenu->addAction("Save int. (and fft. if short scan) at this pixel on next measurement", this, SLOT(onSavePixData()));
    clickMenu->addAction("Save image to file", this, SLOT(onSaveCameraPicture()));
    clickMenu->addAction("Reset Scan/Focus ROI", this, SLOT(onResetROI()));

    clickMenuSelection=new QMenu;
    clickMenuSelection->addAction("Set Scan/Focus ROI", this, SLOT(onChangeROI()));
    clickMenuSelection->addAction("Save image to file", this, SLOT(onSaveCameraPicture()));

    clickMenuDepthRight=new QMenu;
    clickMenuDepthRight->addAction("Save DepthMap (wtih border, scalebar and colorbar) to file", this, SLOT(onSaveDepthMap()));
    clickMenuDepthRight->addAction("Save DepthMap (raw, float) to file", this, SLOT(onSaveDepthMapRaw()));
    clickMenuDepthRight->addAction("Save DepthMap (txt, float) to file", this, SLOT(onSaveDepthMapTxt()));
    clickMenuDepthRight->addAction("Plot Rect (Gnuplot)", this, SLOT(onPlotRect()));
    clickMenuDepthRight->addAction("Rotate DepthMap", this, SLOT(onRotateDepthMap()));
    clickMenuDepthRight->addAction("Crop", this, SLOT(onCrop()));
    clickMenuDepthRightTilt=clickMenuDepthRight->addMenu("Correct tilt");
        clickMenuDepthRightTilt->addAction("XY plane", this, SLOT(onCorrectTiltXY()));
        clickMenuDepthRightTilt->addAction("X axis", this, SLOT(onCorrectTiltX()));
        clickMenuDepthRightTilt->addAction("Y axis", this, SLOT(onCorrectTiltY()));
    clickMenuDepthRight->addAction("2D FFT", this, SLOT(on2DFFT()));
    clickMenuDepthRight->addAction("Apply xy sobel", this, SLOT(onSobel()));
    clickMenuDepthRight->addAction("Apply laplace", this, SLOT(onLaplace()));
    clickMenuDepthRight->addAction("Fit peak", this, SLOT(onPeakFit()));

    clickMenuDepthLeft=new QMenu;
    clickMenuDepthLeft->addAction("Plot Line (Gnuplot)", this, SLOT(onPlotLine()));
    clickMenuDepthLeft->addAction("Save Line (.txt)", this, SLOT(onSaveLine()));

    conf.load();    //TODO remove
}

tab_camera::~tab_camera(){
    if(conf.changed()) conf.save();    //TODO remove
    delete scanRes;
    delete pgHistGUI;
    delete pgCor;
    delete pgCal;
    delete pgSGUI;
    delete pgBeAn;
    delete pgMGUI;
    delete pgTGUI;
    delete pgFGUI;
    delete pgPRGUI;
    go.pRPTY->setGPIO("ilumLED", 0);
}

void tab_camera::work_fun(){
    if(selDisp->index==0) framequeueDisp->setUserFps(camSet->guiCamFPS->val,5);
    else framequeueDisp->setUserFps(0);
    onDisplay=framequeueDisp->getUserMat();


    if(selDisp->index==0){  // Camera
        LDisplay->isDepth=false;
        if(onDisplay!=nullptr){
            long ipos[2];
            bool hasOverlay=false;
            if(camSet->isMirau)
                for(auto& _ovl:ovl){
                    if(_ovl.enabled)if(!_ovl.empty()){
                        hasOverlay=true;
                        break;
                    }
                }
            if(hasOverlay){
                double pos[2];
                pgMGUI->getPos(&pos[0],&pos[1]);
                for(int i:{0,1}) ipos[i]=static_cast<long>(pgMGUI->mm2px(pos[i]));
                for(auto& _ovl:ovl) _ovl.visible=_ovl.enabled?_ovl.check(*onDisplay,ipos[0],ipos[1]):false;
            }

            if(hasOverlay || main_show_target->isChecked() || main_show_scale->isChecked() || selectingFlag || lastSelectingFlag || sROI.width!=0){
                cv::Mat temp=onDisplay->clone();
                if(main_CLAHE_writing->isChecked() && !camSet->isMirau){
                    auto CLAHE=cv::createCLAHE(camSet->CLAHE_clipLimit->val, {(int)camSet->CLAHE_tileGridSize->val,(int)camSet->CLAHE_tileGridSize->val});
                    CLAHE->apply(temp,temp);
                }
                if(hasOverlay) for(auto& _ovl:ovl) if(_ovl.visible) _ovl.drawOverlays(temp,ipos[0],ipos[1]);
                if(main_show_target->isChecked()) cMap->draw_bw_target(&temp, pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX), pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY));
                if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp, pgMGUI->getNmPPx(), pgMGUI->currentObj==1?(static_cast<double>(camSet->wrsbar_unit->val)):0);
                if(selectingFlag){
                    if(selectingFlagIsLine) cv::line(temp, {static_cast<int>(selStartX+1),static_cast<int>(selStartY+(temp.rows-temp.rows)/2)},{static_cast<int>(selCurX+1),static_cast<int>(selCurY+(temp.rows-temp.rows)/2)}, cv::Scalar{255}, 1, cv::LINE_AA);
                    else cv::rectangle(temp, {static_cast<int>(selStartX+1),static_cast<int>(selStartY+(temp.rows-temp.rows)/2)},{static_cast<int>(selCurX+1),static_cast<int>(selCurY+(temp.rows-temp.rows)/2)}, cv::Scalar{255}, 1);
                }
                if(camSet->isMirau && sROI.width!=0) cv::rectangle(temp, sROI, cv::Scalar{255}, 1);

                pgCal->drawWriteArea(&temp);
                pgWrt->drawWriteArea(&temp);
                scaleDisplay(temp, QImage::Format_Indexed8);
//                cv::applyColorMap(temp, temp, OCV_CM::ids[cm_sel->index]);
//                cv::cvtColor(temp, temp, cv::COLOR_BGR2RGB);
//                scaleDisplay(temp, QImage::Format_RGB888);
            }
            else scaleDisplay(*onDisplay, QImage::Format_Indexed8);
            addInfo->setVisible(false);
        }
        if(scanRes->changed() || cm_sel->index!=oldCm || redrawHistClrmap || pgHistGUI->changed || updateDisp){
            const pgScanGUI::scanRes* res;
            if(scanRes->changed()) loadedOnDisplay=false;
            if(loadedOnDisplay) res=&loadedScan;
            else res=scanRes->get();
            pgHistGUI->updateImg(res);
            oldCm=cm_sel->index;
        }
    }else if(selDisp->index==1 || selDisp->index==2 || selDisp->index==3){   // Depth map or SD or refl.
        LDisplay->isDepth=true;
        if(scanRes->changed() || oldIndex!=selDisp->index || cm_sel->index!=oldCm || redrawHistClrmap || pgHistGUI->changed || cMap->changed || selectingFlag || lastSelectingFlag || updateDisp){
            const pgScanGUI::scanRes* res;
            if(scanRes->changed()) loadedOnDisplay=false;
            if(loadedOnDisplay) res=&loadedScan;
            else res=scanRes->get();

            if(res!=nullptr){
                double min,max;
                cv::Mat display;
                if(selDisp->index==1 || (selDisp->index==2 && res->depthSS.empty()) || (selDisp->index==3 && res->refl.empty())){  //show Depth Map
                    pgHistGUI->updateImg(res, &min, &max);
                    cMap->colormappize(&res->depth, &display, &res->mask, min, max, res->XYnmppx, pgHistGUI->outOfRangeToExcl->val);
                }else if(selDisp->index==2){                //show SD
                    cv::divide(res->depthSS,res->avgNum-1,display);
                    double _min,_max;
                    cv::sqrt(display,display);
                    cv::minMaxLoc(display, &_min, &_max, nullptr, nullptr, res->maskN);
                    pgHistGUI->updateImg(res, &min, &max, 0, _max, &display);
                    cMap->colormappize(&display, &display, &res->mask, 0, max, res->XYnmppx, pgHistGUI->outOfRangeToExcl->val, false, "SD (nm)");
                }else if(selDisp->index==3){                //show reflectivity
                    res->refl.copyTo(display);
                    double min,max; cv::minMaxIdx(display,&min,&max);
                    pgHistGUI->updateImg(res, nullptr, nullptr, min, max, &display);
                    cv::Mat tempMask(display.rows, display.cols, CV_8U, cv::Scalar(0));
                    cMap->colormappize(&display, &display, &tempMask, min, max, res->XYnmppx, false, false, "Reflectivity", true);
                }
                dispDepthMatRows=res->depth.rows;
                dispDepthMatCols=res->depth.cols;

                if(selectingFlag){
                    if(selectingFlagIsLine) cv::line(display, {static_cast<int>(selStartX+1),static_cast<int>(selStartY+(display.rows-res->depth.rows)/2)},{static_cast<int>(selCurX+1),static_cast<int>(selCurY+(display.rows-res->depth.rows)/2)}, cv::Scalar{exclColor.val[2],exclColor.val[1],exclColor.val[0],255}, 1, cv::LINE_AA);
                    else cv::rectangle(display, {static_cast<int>(selStartX+1),static_cast<int>(selStartY+(display.rows-res->depth.rows)/2)},{static_cast<int>(selCurX+1),static_cast<int>(selCurY+(display.rows-res->depth.rows)/2)}, cv::Scalar{exclColor.val[2],exclColor.val[1],exclColor.val[0],255}, 1);
                }

                scaleDisplay(display, QImage::Format_RGBA8888);
                if(res->tiltCor[0]!=0 || res->tiltCor[1]!=0 || res->avgNum>1){
                    std::string text;
                    if(res->tiltCor[0]!=0 || res->tiltCor[1]!=0) text+=util::toString("Tilt correction was: X: ",res->tiltCor[0],", Y: ",res->tiltCor[1],"\n");
                    text+=util::toString("Coords were: X: ",res->pos[0],", Y: ",res->pos[1],", Z: ",res->pos[2],"\n");
                    if(res->avgNum>1) text+=util::toString("Averaged ",res->avgNum," measurements.");
                    addInfo->setText(QString::fromStdString(text));
                    addInfo->setVisible(true);
                } else addInfo->setVisible(false);

            }
            oldCm=cm_sel->index;
        }
    }
    oldIndex=selDisp->index;
    if(onDisplay!=nullptr) framequeueDisp->freeUserMat();
    lastSelectingFlag=selectingFlag;
    redrawHistClrmap=false;
    updateDisp=false;

    measPB->setValue(MLP.progress_proc);
    compPB->setValue(MLP.progress_comp);
}

void iImageDisplay::resizeEvent(QResizeEvent *event){
    parent->updateDisp=true;
}

void tab_camera::scaleDisplay(cv::Mat img, QImage::Format format){
    cv::Mat tmp;

    double _s;
    ld_scale=1;
    _s=((double)LDisplay->geometry().width()-20)/img.cols;
    if(ld_scale>_s) ld_scale=_s;
    _s=((double)LDisplay->geometry().height()-20)/img.rows;
    if(ld_scale>_s) ld_scale=_s;

    if(ld_scale!=1) cv::resize(img, tmp, cv::Size(0,0),ld_scale,ld_scale, cv::INTER_AREA);
    else tmp=img;
    LDisplay->setPixmap(QPixmap::fromImage(QImage(tmp.data, tmp.cols, tmp.rows, tmp.step, format)));
}

void tab_camera::on_tab_change(int index){
    if(index==index_pgSGUI) Q_EMIT pgSGUI->recalculate();
    else if(index==index_pgFGUI) Q_EMIT pgFGUI->recalculate();
    else if(index==index_camSet) Q_EMIT camSet->genReport();
}
void tab_camera::on_EP_sel_released(){
    QColor color=QColorDialog::getColor(Qt::white, this, "Select excluded pixel color");
    exclColor.val[2]=color.red();
    exclColor.val[1]=color.green();
    exclColor.val[0]=color.blue();
    redrawHistClrmap=true;
}
void tab_camera::onShowAbsHeightChanged(){
    redrawHistClrmap=true;
}

void tab_camera::tab_entered(){
    framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeueDisp->setUserFps(camSet->guiCamFPS->val,5);
    go.pRPTY->setGPIO("ilumLED", 1);

    timer->start(work_call_time);
}
void tab_camera::tab_exited(){
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
    timer->stop();
}



//  iImageDisplay EVENT HANDLING

void iImageDisplay::calsVars(QMouseEvent *event, double* xcoord, double* ycoord, double* pwidth, double* pheight){
    *xcoord=event->pos().x()-(width()-pixmap()->width())/2;
    *ycoord=event->pos().y()-(height()-pixmap()->height())/2;
    if(pwidth!=nullptr) *pwidth=pixmap()->width();
    *pheight=pixmap()->height();

    if(parent->ld_scale!=1){
        double scale=parent->ld_scale;
        if(parent->selDisp->index!=0){  // not camera
            if(go.pGCAM->iuScope->connected){
                const pgScanGUI::scanRes* res;
                if(parent->loadedOnDisplay) res=&parent->loadedScan;
                else res=parent->scanRes->get();
                double colRatio=static_cast<double>(res->depth.cols)/go.pGCAM->iuScope->camCols;
                double rowRatio=static_cast<double>(res->depth.rows)/go.pGCAM->iuScope->camRows;
                if(colRatio<1 && rowRatio<1){
                    if(colRatio<=scale && rowRatio<=scale) scale=1;
                    else if(colRatio>rowRatio) scale/=colRatio;
                    else if(colRatio<rowRatio) scale/=rowRatio;
                }
            }
        }
        *xcoord/=scale;
        *ycoord/=scale;
        if(pwidth!=nullptr) *pwidth/=scale;
        *pheight/=scale;
    }
}

void iImageDisplay::mouseMoveEvent(QMouseEvent *event){
    double xcoord,ycoord,pxH;
    calsVars(event, &xcoord, &ycoord, nullptr, &pxH);
    parent->selCurX=xcoord;
    parent->selCurY=ycoord;
    if(isDepth){
        parent->selCurX-=1;                                 //correct for drawn margins on image
        parent->selCurY-=(pxH-parent->dispDepthMatRows)/2;
        if(parent->selectingFlag && parent->selectingFlagIsLine){
            if(event->modifiers()&Qt::ControlModifier){  //ctrl is held
                if(abs(parent->selCurX-parent->selStartX)>abs(parent->selCurY-parent->selStartY)) parent->selCurY=parent->selStartY;
                else parent->selCurX=parent->selStartX;
            }
        }
    }
    else{}
}
void iImageDisplay::mousePressEvent(QMouseEvent *event){
    double xcoord,ycoord,pxH;
    calsVars(event, &xcoord, &ycoord, nullptr, &pxH);
    parent->selStartX=xcoord; parent->selCurX=xcoord;
    parent->selStartY=ycoord; parent->selCurY=ycoord;
    if(isDepth){
        parent->selCurX-=1;                                 //correct for drawn margins on image
        parent->selCurY-=(pxH-parent->dispDepthMatRows)/2;
        parent->selStartX-=1;
        parent->selStartY-=(pxH-parent->dispDepthMatRows)/2;

        if(event->button()==Qt::RightButton){
            parent->selectingFlag=true;
            parent->selectingFlagIsLine=false;
        }else if(event->button()==Qt::LeftButton){
            parent->selectingFlag=true;
            parent->selectingFlagIsLine=true;
        }
    }
    else{
        if(event->button()==Qt::RightButton){
            parent->selectingFlag=true;
            parent->selectingFlagIsLine=false;
        }
    }
}
void iImageDisplay::mouseReleaseEvent(QMouseEvent *event){
    double xcoord,ycoord,pxW,pxH;
    calsVars(event, &xcoord, &ycoord, &pxW, &pxH);
    parent->selEndX=xcoord;
    parent->selEndY=ycoord;

    if(isDepth){
        parent->selEndX-=1;                                 //correct for drawn margins on image
        parent->selEndY-=(pxH-parent->dispDepthMatRows)/2;

        if((parent->selStartX<0 && parent->selEndX<0) || (parent->selStartX>=parent->dispDepthMatCols && parent->selEndX>=parent->dispDepthMatCols) || (parent->selStartY<0 && parent->selEndY<0) || (parent->selStartY>=parent->dispDepthMatRows && parent->selEndY>=parent->dispDepthMatRows))
            {parent->selectingFlag=false; return;}
        if(parent->selStartX<0) parent->selStartX=0;
        if(parent->selEndX<0) parent->selEndX=0;
        if(parent->selStartX>=parent->dispDepthMatCols) parent->selStartX=parent->dispDepthMatCols-1;
        if(parent->selEndX>=parent->dispDepthMatCols) parent->selEndX=parent->dispDepthMatCols-1;
        if(parent->selStartY<0) parent->selStartY=0;
        if(parent->selEndY<0) parent->selEndY=0;
        if(parent->selStartY>=parent->dispDepthMatRows) parent->selStartY=parent->dispDepthMatRows-1;
        if(parent->selEndY>=parent->dispDepthMatRows) parent->selEndY=parent->dispDepthMatRows-1;

        if(event->button()==Qt::RightButton){
            parent->clickMenuDepthRight->popup(QCursor::pos());
            parent->selectingFlag=false;
        }else if(event->button()==Qt::LeftButton){
            if(event->modifiers()&Qt::ControlModifier){  //ctrl is held
                if(abs(parent->selEndX-parent->selStartX)>abs(parent->selEndY-parent->selStartY)) parent->selEndY=parent->selStartY;
                else parent->selEndX=parent->selStartX;
            }
            parent->clickMenuDepthLeft->popup(QCursor::pos());
            parent->selectingFlag=false;
        }
    }
    else{
        if(xcoord<0 || xcoord>=pxW || ycoord<0 || ycoord>=pxH) {parent->selectingFlag=false; return;} //ignore events outside pixmap;
        if(event->button()==Qt::LeftButton){
            double DX, DY;
            DX=parent->pgMGUI->px2mm(xcoord-pxW/2.)+parent->pgBeAn->writeBeamCenterOfsX;
            DY=parent->pgMGUI->px2mm(pxH/2.-ycoord)+parent->pgBeAn->writeBeamCenterOfsY;
            parent->pgMGUI->move(DX,DY,0,false,!parent->camSet->isMirau);
            if(parent->pgMGUI->reqstNextClickPixDiff) parent->pgMGUI->delvrNextClickPixDiff(xcoord-pxW/2, pxH/2-ycoord);
            parent->sROI.width=0;
        }else if(event->button()==Qt::RightButton){
            if(parent->selStartX==parent->selEndX || parent->selStartY==parent->selEndY) parent->clickMenu->popup(QCursor::pos());
            else parent->clickMenuSelection->popup(QCursor::pos());
            parent->selectingFlag=false;
        }
    }
}
void iImageDisplay::wheelEvent(QWheelEvent *event){
    if(isDepth){}
    else{
        parent->pgMGUI->scaledMoveZ(10*event->delta());
    }
}


void tab_camera::onSavePixData(void){
    QString fileName=QFileDialog::getSaveFileName(this,"Select text file for saving pixel data.", "","Text (*.txt *.dat *.csv)");
    if(fileName.isEmpty())return;
    std::lock_guard<std::mutex>lock(pgSGUI->clickDataLock);
    pgSGUI->savePix=true;
    pgSGUI->clickFilename=fileName.toStdString();
    if(pgSGUI->clickFilename.find(".txt")==std::string::npos && pgSGUI->clickFilename.find(".dat")==std::string::npos && pgSGUI->clickFilename.find(".csv")==std::string::npos) pgSGUI->clickFilename+=".txt";
    pgSGUI->clickCoordX=selEndX;
    pgSGUI->clickCoordY=selEndY;
}
void tab_camera::onSaveCameraPicture(void){
    std::string fileName=QFileDialog::getSaveFileName(this,"Select file for saving Camera Picture.", "","Images (*.png)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".png")==std::string::npos) fileName+=".png";
    FQ* fq=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    fq->setUserFps(999,1);
    while(fq->getUserMat()==nullptr);
    cv::Mat temp=onDisplay->clone();
    if(main_show_target->isChecked()) cMap->draw_bw_target(&temp, pgMGUI->px2mm(pgBeAn->writeBeamCenterOfsX), pgMGUI->px2mm(pgBeAn->writeBeamCenterOfsY));
    if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp, pgMGUI->getNmPPx(), pgMGUI->currentObj==1?(static_cast<double>(camSet->wrsbar_unit->val)):0);

    if(selStartX!=selEndX && selStartY!=selEndY){
        int width=abs(selEndX-selStartX+1);
        int height=abs(selEndY-selStartY+1);
        temp=temp({static_cast<int>(selStartX<selEndX?selStartX:(selStartX-width)), static_cast<int>(selStartY<selEndY?selStartY:(selStartY-height)), width, height});
    }
    imwrite(fileName, temp,{cv::IMWRITE_PNG_COMPRESSION,9});
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(fq);
}
void tab_camera::onResetROI(){
    sROI.width=0;
}
void tab_camera::onChangeROI(){
    sROI.width=abs(selEndX-selStartX+1);
    sROI.height=abs(selEndY-selStartY+1);
    sROI.x=selStartX<selEndX?selStartX:selEndX;
    sROI.y=selStartY<selEndY?selStartY:selEndY;
}
void tab_camera::onSaveDepthMap(void){
    std::string fileName=QFileDialog::getSaveFileName(this,"Select file for saving Depth Map (wtih border, scalebar and colorbar).", "","Images (*.png)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".png")==std::string::npos) fileName+=".png";
    double min,max;
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();

    if(res!=nullptr){
        cv::Mat display;
        int width=abs(selEndX-selStartX+1);
        int height=abs(selEndY-selStartY+1);
        if(selDisp->index==1 || (selDisp->index==2 && res->depthSS.empty()) || (selDisp->index==3 && res->refl.empty())){
            pgHistGUI->updateImg(res, &min, &max);
            if(width>1 && height>1){
                cv::Mat temp0(res->depth,{static_cast<int>(selStartX<selEndX?selStartX:(selStartX-width)), static_cast<int>(selStartY<selEndY?selStartY:(selStartY-height)), width, height});
                cv::Mat temp1(res->mask ,{static_cast<int>(selStartX<selEndX?selStartX:(selStartX-width)), static_cast<int>(selStartY<selEndY?selStartY:(selStartY-height)), width, height});
                cv::Mat temp2; bitwise_not(temp1, temp2);
                double minr, maxr;
                cv::minMaxLoc(temp0, &minr, &maxr, nullptr, nullptr, temp2);
                cMap->colormappize(&temp0, &display, &temp1, std::min(std::max(min,minr), std::min(max,maxr)), std::max(std::min(max,maxr),std::max(min,minr)), res->XYnmppx, pgHistGUI->outOfRangeToExcl->val, true);
            }
            else cMap->colormappize(&res->depth, &display, &res->mask, min, max, res->XYnmppx, pgHistGUI->outOfRangeToExcl->val, !*cMap->exportSet4WholeVal);
        }else if(selDisp->index==2){
            cv::divide(res->depthSS,res->avgNum-1,display);
            double _min,_max; cv::Point ignore;
            cv::sqrt(display,display);
            cv::minMaxLoc(display, &_min, &_max, &ignore, &ignore, res->maskN);
            pgHistGUI->updateImg(res, &min, &max, 0, _max, &display);
            if(width>1 && height>1){
                cv::Mat temp0(display,{static_cast<int>(selStartX<selEndX?selStartX:(selStartX-width)), static_cast<int>(selStartY<selEndY?selStartY:(selStartY-height)), width, height});
                cv::Mat temp1(res->mask ,{static_cast<int>(selStartX<selEndX?selStartX:(selStartX-width)), static_cast<int>(selStartY<selEndY?selStartY:(selStartY-height)), width, height});
                cv::Mat temp2; bitwise_not(temp1, temp2);
                double minr, maxr;
                cv::minMaxLoc(temp0, &minr, &maxr, nullptr, nullptr, temp2);
                cMap->colormappize(&temp0, &display, &temp1, 0, maxr, res->XYnmppx, pgHistGUI->outOfRangeToExcl->val, true, "SD (nm)");
            }
            else cMap->colormappize(&display, &display, &res->mask, 0, max, res->XYnmppx, pgHistGUI->outOfRangeToExcl->val, !*cMap->exportSet4WholeVal, "SD (nm)");
        }else if(selDisp->index==3){                //show reflectivity
            cv::Mat temp0;
            if(width>1 && height>1) temp0=cv::Mat(res->refl,{static_cast<int>(selStartX<selEndX?selStartX:(selStartX-width)), static_cast<int>(selStartY<selEndY?selStartY:(selStartY-height)), width, height});
            else                    temp0=res->refl;
            cv::minMaxIdx(temp0,&min,&max);
            cv::Mat tempMask(temp0.rows, temp0.cols, CV_8U, cv::Scalar(0));
            cMap->colormappize(&temp0, &display, &tempMask, min, max, res->XYnmppx, false, !*cMap->exportSet4WholeVal, "Reflectivity", true);
        }

        cv::cvtColor(display, display, cv::COLOR_RGBA2BGRA);
        imwrite(fileName, display,{cv::IMWRITE_PNG_COMPRESSION,9});
    }
}
void tab_camera::onSaveDepthMapRaw(bool txt){
    int width=abs(selEndX-selStartX+1);
    int height=abs(selEndY-selStartY+1);
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    if(res==nullptr) return;
    if(width>1 && height>1){
           if(txt)  pgScanGUI::saveScanTxt(res, cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
           else     pgScanGUI::saveScan(res, cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
    }else{
        if(txt)  pgScanGUI::saveScanTxt(res);
        else     pgScanGUI::saveScan(res);
    }
}
void tab_camera::onLoadDepthMapRaw(void){
    if(pgScanGUI::loadScan(&loadedScan)){
        updateDisp=true;
        loadedOnDisplay=true;
    }
}
void tab_camera::showScan(pgScanGUI::scanRes scan){
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);    //wait for tab_camera to detect new scan, and then send signal
    loadedScan=scan;
    updateDisp=true;
    loadedOnDisplay=true;
}
void tab_camera::onDiff2Raw(){
    if(pgScanGUI::loadScan(&scanBefore) && pgScanGUI::loadScan(&scanAfter))
        diff2RawCompute();
}
void tab_camera::onAvgNRawBtn(){
    std::vector<std::filesystem::path> readFolders;
    std::vector<std::filesystem::path> measFiles;

    std::string path=QFileDialog::getExistingDirectory(this, "Select Folder Containing Scans. It will be Searched Recursively.").toStdString();
    if(path.empty()) return;
    readFolders.push_back(std::filesystem::path(path));
    while(!readFolders.empty()){
        std::vector<std::filesystem::directory_entry> files;
        for(auto const& dir_entry: std::filesystem::directory_iterator{readFolders.back()}) files.push_back(dir_entry);
        readFolders.pop_back();
        for(auto& path: files){
            if(path.is_directory()) readFolders.push_back(path);
            else if(path.path().stem().string().find("-SD")==std::string::npos && path.path().extension().string()==".pfm") measFiles.push_back(path);
        }
    }
    //for(auto& path: measFiles) std::cerr<<path.filename().string()<<"\n";
    if(measFiles.empty()) return;
    std::vector<pgScanGUI::scanRes> scans;
    for(auto& path: measFiles){
        scans.emplace_back();
        pgScanGUI::loadScan(&scans.back(),path.string());
        if(avgNRawMarginCut->val>0){
            int margin=avgNRawMarginCut->val;
            cv::Mat tmp;
            int width=scans.back().depth.cols-2*margin;
            int height=scans.back().depth.rows-2*margin;
            if(width<=0 || height <=0){
                std::cerr<<"Cut margins are too large!\n";
                return;
            }
            scans.back().depth(cv::Rect(margin, margin, width, height)).copyTo(tmp);
            tmp.copyTo(scans.back().depth);
            scans.back().mask(cv::Rect(margin, margin, width, height)).copyTo(tmp);
            tmp.copyTo(scans.back().mask);
            bitwise_not(scans.back().mask, scans.back().maskN);
            scans.back().depthSS.release();
            scans.back().refl.release();
            cv::minMaxLoc(scans.back().depth, &scans.back().min, &scans.back().max, nullptr, nullptr, scans.back().maskN);
        }
    }

    pgScanGUI::scanRes res;
    if(avgNRawMarginRef->val>0){
        int margin=avgNRawMarginRef->val;
        int width=scans.back().depth.cols-2*margin;
        int height=scans.back().depth.rows-2*margin;
        if(width<=0 || height <=0){
            std::cerr<<"Ref margins are too large!\n";
            return;
        }
        cv::Mat refMask=cv::Mat::zeros(scans.back().depth.size(),CV_8U);
        refMask.rowRange(0,margin).setTo(255);
        refMask.rowRange(refMask.cols-margin,refMask.cols).setTo(255);
        refMask.colRange(0,margin).setTo(255);
        refMask.colRange(refMask.cols-margin,refMask.cols).setTo(255);
        res=pgSGUI->avgScans(&scans,-1,10,&refMask);
    }else{
        res=pgSGUI->avgScans(&scans);
    }

    if(res.depth.empty()) return;
    res.copyTo(loadedScan);
    updateDisp=true;
    loadedOnDisplay=true;
}


void tab_camera::diff2RawCompute(){
    if(scanBefore.depth.empty() || scanAfter.depth.empty()) return;
    pgScanGUI::scanRes scanDif=pgSGUI->difScans(&scanBefore, &scanAfter);
    if(scanDif.depth.empty()) return;
    loadedScan=scanDif;
    updateDisp=true;
    loadedOnDisplay=true;
}

void tab_camera::onCombineMes(){
    QStringList files=QFileDialog::getOpenFileNames(this,"Select depthmaps to combine (files with -SD are ignored)","","Depthmaps (*.pfm)");
    std::deque<pgScanGUI::scanRes> scans;
    while(!files.empty()){
        if(files.back().toStdString().find("-SD")==std::string::npos){
            std::cout<<"loading "<<files.back().toStdString()<<"\n";
            scans.emplace_back();
            if(!pgScanGUI::loadScan(&scans.back(),files.back().toStdString())) scans.pop_back();
        }
        files.pop_back();
    }
    std::cout<<"Loaded in total "<<scans.size()<<" depthmaps.\n";
    if(scans.size()<=1) return;
    double minX=std::numeric_limits<double>::max(),minY=minX,maxX=std::numeric_limits<double>::lowest(),maxY=maxX; // NOTE: the values in X/Y mm go in opposite direction from matrix col/row
    double tmp;
    for(int i=0;i!=scans.size();i++){
        if(maxX<scans[i].pos[0]) maxX=scans[i].pos[0];
        if(maxY<scans[i].pos[1]) maxY=scans[i].pos[1];
        tmp=scans[i].pos[0]-scans[i].depth.cols*scans[i].XYnmppx/1e6;
        if(minX>tmp) minX=tmp;
        tmp=scans[i].pos[1]-scans[i].depth.rows*scans[i].XYnmppx/1e6;
        if(minY>tmp) minY=tmp;
    }
    cv::Size newSize((int)ceil((maxX-minX)*1e6/scans[0].XYnmppx),(int)ceil((maxY-minY)*1e6/scans[0].XYnmppx));
    std::cout<<"New mat size: "<<newSize<<"\n";
    cv::Mat outMat(newSize, CV_8UC4, cv::Scalar(0,0,0,0));
    for(int i=0;i!=scans.size();i++){
        int sX=(maxX-scans[i].pos[0])*1e6/scans[i].XYnmppx;
        int sY=(maxY-scans[i].pos[1])*1e6/scans[i].XYnmppx;
        cv::Mat res;
        if(combineUseRefl->isChecked()){
            double min,max; cv::minMaxIdx(scans[i].refl,&min, &max);
            cv::Mat tmpMask(scans[i].refl.size(), CV_8U, cv::Scalar(0));
            cMap->colormappize(&scans[i].refl, &res, &tmpMask, min, max, scans[i].XYnmppx, pgHistGUI->outOfRangeToExcl->val);
        }else cMap->colormappize(&scans[i].depth, &res, &scans[i].mask, scans[i].min, scans[i].max, scans[i].XYnmppx, pgHistGUI->outOfRangeToExcl->val);
        res(cv::Rect(1,(res.rows-scans[i].depth.rows)/2,scans[i].depth.cols,scans[i].depth.rows)).copyTo(outMat(cv::Rect(sX,sY,scans[i].depth.cols,scans[i].depth.rows)));
    }
    std::string fileName=QFileDialog::getSaveFileName(this,"Select file for saving Depth Map (wtih border, scalebar and colorbar).", "","Images (*.png)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".png")==std::string::npos) fileName+=".png";
    imwrite(fileName, outMat,{cv::IMWRITE_PNG_COMPRESSION,9});
}
void tab_camera::onRotateDepthMap(){
    double angle=QInputDialog::getDouble(this, "Rotate Depth Map", "Rotation Angle (degrees):", 0, -360, 360, 2);
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    int rows=loadedScan.depth.rows;
    int cols=loadedScan.depth.cols;
    cv::Point2f center((selEndX+selStartX)/2, (selEndY+selStartY)/2);

    cv::Point2i pt[4]{{0,0},{0,rows},{cols,0},{cols,rows}};
    cv::Point2i ptr[4];
    double angleRad=-angle*M_PI/180;
    for(int i=0;i!=4;i++){
        double r=sqrt(pow((pt[i].x-center.x),2)+pow((pt[i].y-center.y),2));
        double p=std::atan2(pt[i].y-center.y,pt[i].x-center.x);
        ptr[i]={static_cast<int>(r*cos(p+angleRad)+center.x),static_cast<int>(r*sin(p+angleRad)+center.y)};
    }
    int rowPadBtm=-std::min(ptr[0].y,std::min(ptr[1].y,std::min(ptr[2].y,ptr[3].y)));
    int rowPadTop=std::max(ptr[0].y,std::max(ptr[1].y,std::max(ptr[2].y,ptr[3].y)))-rows;
    int colPadBtm=-std::min(ptr[0].x,std::min(ptr[1].x,std::min(ptr[2].x,ptr[3].x)));
    int colPadTop=std::max(ptr[0].x,std::max(ptr[1].x,std::max(ptr[2].x,ptr[3].x)))-cols;
    if(rowPadBtm<0) rowPadBtm=0;
    if(rowPadTop<0) rowPadTop=0;
    if(colPadBtm<0) colPadBtm=0;
    if(colPadTop<0) colPadTop=0;
    cv::copyMakeBorder(loadedScan.depth, loadedScan.depth, rowPadBtm, rowPadTop, colPadBtm, colPadTop, cv::BORDER_CONSTANT,cv::Scalar(loadedScan.min));
    cv::copyMakeBorder(loadedScan.mask, loadedScan.mask, rowPadBtm, rowPadTop, colPadBtm, colPadTop, cv::BORDER_CONSTANT,cv::Scalar(255));
    double minSS,minRef;
    if(!loadedScan.depthSS.empty()){
        cv::minMaxLoc(loadedScan.depthSS, &minSS, nullptr, nullptr, nullptr, loadedScan.maskN);
        cv::copyMakeBorder(loadedScan.depthSS, loadedScan.depthSS, rowPadBtm, rowPadTop, colPadBtm, colPadTop, cv::BORDER_CONSTANT,cv::Scalar(minSS));
    }
    if(!loadedScan.refl.empty()){
        cv::minMaxLoc(loadedScan.refl, &minRef, nullptr, nullptr, nullptr, loadedScan.maskN);
        cv::copyMakeBorder(loadedScan.refl, loadedScan.refl, rowPadBtm, rowPadTop, colPadBtm, colPadTop, cv::BORDER_CONSTANT,cv::Scalar(minRef));
    }

    bitwise_not(loadedScan.mask, loadedScan.maskN);
    center.x+=colPadBtm;
    center.y+=rowPadBtm;

    loadedScan.depth.setTo(loadedScan.min,loadedScan.mask);              // this is to get rid of infinite edges that are missed by the mask
    cv::Mat TM=cv::getRotationMatrix2D(center, angle, 1.0);
    cv::warpAffine(loadedScan.maskN, loadedScan.maskN, TM, loadedScan.maskN.size());        // this first so that the rest gets filled with zeros, ie bad pixels
    cv::compare(loadedScan.maskN, cv::Scalar(255), loadedScan.mask, cv::CMP_LT);            // spread the nask
    bitwise_not(loadedScan.mask, loadedScan.maskN);
    cv::warpAffine(loadedScan.depth, loadedScan.depth, TM, loadedScan.depth.size());
    loadedScan.depth.setTo(std::numeric_limits<float>::max(),loadedScan.mask);
    if(!loadedScan.depthSS.empty())
        cv::warpAffine(loadedScan.depthSS, loadedScan.depthSS, TM, loadedScan.depthSS.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(minSS));
    if(!loadedScan.refl.empty())
        cv::warpAffine(loadedScan.refl, loadedScan.refl, TM, loadedScan.refl.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(minRef));
    cv::minMaxLoc(loadedScan.depth, &loadedScan.min, &loadedScan.max, nullptr, nullptr, loadedScan.maskN);

    selEndX=selStartX;  // auto crop
    onCrop();

    updateDisp=true;
    loadedOnDisplay=true;
}

void tab_camera::onCrop(){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    if(selEndX==selStartX || selEndY==selStartY){
        selStartX=0;
        selStartY=0;
        selEndX=loadedScan.depth.cols-1;
        selEndY=loadedScan.depth.rows-1;

        while((selStartX!=selEndX) && (cv::countNonZero(loadedScan.maskN.col(selStartX))<1)) selStartX++;
        while((selStartY!=selEndY) && (cv::countNonZero(loadedScan.maskN.row(selStartY))<1)) selStartY++;
        while((selEndX!=selStartX) && (cv::countNonZero(loadedScan.maskN.col(selEndX))<1)) selEndX--;
        while((selEndY!=selStartY) && (cv::countNonZero(loadedScan.maskN.row(selEndY))<1)) selEndY--;
    }

    cv::Mat tmp;
    int width=abs(selEndX-selStartX+1);
    int height=abs(selEndY-selStartY+1);
    loadedScan.depth(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height)).copyTo(tmp);
    tmp.copyTo(loadedScan.depth);
    loadedScan.mask(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height)).copyTo(tmp);
    tmp.copyTo(loadedScan.mask);
    bitwise_not(loadedScan.mask, loadedScan.maskN);
    if(!loadedScan.depthSS.empty()){
        loadedScan.depthSS(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height)).copyTo(tmp);
        tmp.copyTo(loadedScan.depthSS);
    }
    if(!loadedScan.refl.empty()){
        loadedScan.refl(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height)).copyTo(tmp);
        tmp.copyTo(loadedScan.refl);
    }
    cv::minMaxLoc(loadedScan.depth, &loadedScan.min, &loadedScan.max, nullptr, nullptr, loadedScan.maskN);
    updateDisp=true;
    loadedOnDisplay=true;
}

void tab_camera::onCorrectTilt(int axis){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    if(selStartX==selEndX || selStartY==selEndY){
       pgSGUI->correctTilt(&loadedScan, nullptr, nullptr, axis);
    }else{
        int width=abs(selEndX-selStartX+1);
        int height=abs(selEndY-selStartY+1);
        cv::Mat mask = cv::Mat::zeros(loadedScan.depth.size(), CV_8U);
        cv::rectangle(mask,cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height),255,-1);
        pgSGUI->correctTilt(&loadedScan, &mask, nullptr, axis);
    }

    pgSGUI->applyTiltCorrection(&loadedScan, axis);
    cv::minMaxLoc(loadedScan.depth, &loadedScan.min, &loadedScan.max, nullptr, nullptr, loadedScan.maskN);
    updateDisp=true;
    loadedOnDisplay=true;
}

void tab_camera::onPlotLine(){
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    tCG->plotLine(res,cv::Point(selStartX,selStartY),cv::Point(selEndX,selEndY),selDisp->index);
}
void tab_camera::onSaveLine(){
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    tCG->saveLine(res,cv::Point(selStartX,selStartY),cv::Point(selEndX,selEndY),selDisp->index);
}

void tab_camera::onPlotRect(){
    int width=abs(selEndX-selStartX+1);
    int height=abs(selEndY-selStartY+1);
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    if(res==nullptr) return;
    tCG->plotRoi(res, cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height),selDisp->index);
}

void tab_camera::_cropToSelection(){
    if(selEndX!=selStartX && selEndY!=selStartY){
        int width=abs(selEndX-selStartX+1);
        int height=abs(selEndY-selStartY+1);
        loadedScan.depth=loadedScan.depth(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height));
        loadedScan.mask =loadedScan.mask (cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height));
        loadedScan.maskN=loadedScan.maskN(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height));
        if(!loadedScan.depthSS.empty())
            loadedScan.depthSS=loadedScan.depthSS(cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height));
    }
}
void tab_camera::on2DFFT(){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    _cropToSelection();

    cv::Mat padded;
    int m=cv::getOptimalDFTSize(loadedScan.depth.rows);
    int n=cv::getOptimalDFTSize(loadedScan.depth.cols); // on the border add zero values
    cv::copyMakeBorder(loadedScan.depth, padded, 0, m-loadedScan.depth.rows, 0, n-loadedScan.depth.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));
    cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
    cv::Mat complexI;
    cv::merge(planes, 2, complexI);
    cv::dft(complexI, complexI);
    cv::split(complexI, planes);
    cv::Mat magI, magP;
    cv::magnitude(planes[0], planes[1], magI);
    cv::phase(planes[0], planes[1], magP);
    magI += cv::Scalar::all(1);
    cv::log(magI, magI);
    magI=magI(cv::Rect(0, 0, magI.cols & -2, magI.rows & -2));  // crop
    magP=magP(cv::Rect(0, 0, magP.cols & -2, magP.rows & -2));
    int cx=magI.cols/2;
    int cy=magI.rows/2;

    cv::Mat q0I(magI, cv::Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
    cv::Mat q1I(magI, cv::Rect(cx, 0, cx, cy));  // Top-Right
    cv::Mat q2I(magI, cv::Rect(0, cy, cx, cy));  // Bottom-Left
    cv::Mat q3I(magI, cv::Rect(cx, cy, cx, cy)); // Bottom-Right
    cv::Mat q0P(magP, cv::Rect(0, 0, cx, cy));
    cv::Mat q1P(magP, cv::Rect(cx, 0, cx, cy));
    cv::Mat q2P(magP, cv::Rect(0, cy, cx, cy));
    cv::Mat q3P(magP, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q0I.copyTo(tmp);
    q3I.copyTo(q0I);
    tmp.copyTo(q3I);
    q1I.copyTo(tmp);
    q2I.copyTo(q1I);
    tmp.copyTo(q2I);
    q0P.copyTo(tmp);
    q3P.copyTo(q0P);
    tmp.copyTo(q3P);
    q1P.copyTo(tmp);
    q2P.copyTo(q1P);
    tmp.copyTo(q2P);

    loadedScan.depth=magI;
    loadedScan.mask=cv::Mat(magI.rows,magI.cols,CV_8U,cv::Scalar(0));
    loadedScan.maskN=cv::Mat(magI.rows,magI.cols,CV_8U,cv::Scalar(255));
    cv::multiply(magP,magP,loadedScan.depthSS); loadedScan.avgNum=2;    //these make it display properly as SD gets additional operations performed on it
    cv::minMaxLoc(magI,&loadedScan.min,&loadedScan.max);

    updateDisp=true;
    loadedOnDisplay=true;
}
void tab_camera::onSobel(){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    _cropToSelection();
    const int ksize=5;
    cv::dilate(loadedScan.mask,loadedScan.mask,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(ksize+2,ksize+2), cv::Point((ksize+2)/2,(ksize+2)/2)));
    cv::bitwise_not(loadedScan.mask, loadedScan.maskN);
    cv::Mat sx,sy;
    cv::Sobel(loadedScan.depth,sx, CV_32F,1,0,ksize);
    cv::Sobel(loadedScan.depth,sy, CV_32F,0,1,ksize);
    cv::magnitude(sx,sy,loadedScan.depth);
    cv::minMaxLoc(loadedScan.depth,&loadedScan.min,&loadedScan.max);
    updateDisp=true;
    loadedOnDisplay=true;
}
void tab_camera::onLaplace(){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    _cropToSelection();
    const int ksize=5;
    cv::dilate(loadedScan.mask,loadedScan.mask,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(ksize+2,ksize+2), cv::Point((ksize+2)/2,(ksize+2)/2)));
    cv::bitwise_not(loadedScan.mask, loadedScan.maskN);
    cv::Laplacian(loadedScan.depth,loadedScan.depth, CV_32F,ksize);
    cv::minMaxLoc(loadedScan.depth,&loadedScan.min,&loadedScan.max);
    updateDisp=true;
    loadedOnDisplay=true;
}

void tab_camera::onPeakFit(){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    pgScanGUI::scanRes scan;
    int width=abs(selEndX-selStartX+1);
    int height=abs(selEndY-selStartY+1);
    cv::Rect ROI;
    if(width<=1 || height<=1) ROI={0,0,loadedScan.depth.cols,loadedScan.depth.rows};
    else ROI=cv::Rect(selStartX<selEndX?selStartX:selEndX, selStartY<selEndY?selStartY:selEndY, width, height);
    scan.depth=loadedScan.depth(ROI).clone();
    scan.mask =loadedScan.mask (ROI);
    scan.maskN=loadedScan.maskN(ROI);
    scan.XYnmppx=loadedScan.XYnmppx;
    for(int i:{0,1}) scan.tiltCor[i]=loadedScan.tiltCor[i];
    cv::minMaxIdx(scan.depth,&scan.min,&scan.max, nullptr, nullptr, scan.maskN);
    if(!showAbsHeight->val){
        scan.depth-=scan.min;
        scan.max-=scan.min;
        scan.min=0;
    }



    std::string res;
    cv::Mat difMat;
    pgCal->calcParameters(scan, &res, 0, 0, nullptr, 0, 0, 0, &difMat);
    if(res.empty()) return;

    if(cpwin==nullptr) cpwin=new CvPlotQWindow;
    cpwin->axes=CvPlot::makePlotAxes();
    cpwin->axes=CvPlot::Axes();
    cpwin->axes.create<CvPlot::Border>();
    cpwin->axes.create<CvPlot::XAxis>();
    cpwin->axes.create<CvPlot::YAxis>();
    cpwin->axes.setMargins(80,110,45,45);

    cpwin->axes.xLabel("X (um)");
    cpwin->axes.yLabel("Y (nm)");
    //auto& cb=cpwin->axes.create<CvPlot::CBox>(cv::COLORMAP_TURBO,scan.min,scan.max,scan.min,(scan.max+scan.min)/2);
    double min,max;
    cv::minMaxIdx(difMat,&min,&max, nullptr, nullptr, scan.maskN);
    auto& cb=cpwin->axes.create<CvPlot::CBox>(cv::COLORMAP_TURBO,min,max);
    cb.setLabel("Height error (model-measured) (nm)");
    cpwin->axes.setXTight(true);
    cpwin->axes.setYTight(true);
    auto& sdata=cpwin->axes.create<CvPlot::Image>(difMat);
    sdata.setColormap(cv::COLORMAP_TURBO);
    sdata.setPosition({0,0,difMat.cols*scan.XYnmppx/1000,difMat.rows*scan.XYnmppx/1000});
    sdata.setMinMaxOverride(min, max);

    cpwin->redraw();
    cpwin->show();
    cpwin->activateWindow();    // takes focus
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    QMenu menu;
    if(!fitSaveFilename.empty()) menu.addAction("Append fit results to last chosen file.");
    auto mns=menu.addAction("Make new save file (with header) and append fit results.");
    auto ats=menu.addAction("Append fit results to existing file.");
    auto chs=menu.exec(QCursor::pos());
    if(chs==nullptr) return;
    std::ofstream wfile;
    if     (chs==mns) fitSaveFilename=QFileDialog::getSaveFileName(this, tr("Select file for saving fit results.")   ,fitSaveFilename.c_str(),tr("Text (*.txt *.dat *.csv)")).toStdString();
    else if(chs==ats) fitSaveFilename=QFileDialog::getOpenFileName(this, tr("Select file for appending fit results."),fitSaveFilename.c_str(),tr("Text (*.txt *.dat *.csv)")).toStdString();
    if(fitSaveFilename.empty()) return;
    wfile=std::ofstream(fitSaveFilename, (chs==mns?std::ofstream::trunc:std::ofstream::app));
    if(chs==mns) pgCal->writeProcessHeader(wfile);
    wfile<<res;
    wfile.close();
}
