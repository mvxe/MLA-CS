#include "tab_camera.h"
#include "GUI/mainwindow.h"
#include "includes.h"

tab_camera::tab_camera(QWidget* parent){
    pgScanGUI::parent=this; //for dialogs in pgScanGUI static functions
    layout=new QHBoxLayout;
    parent->setLayout(layout);

    LDisplay=new iImageDisplay;
    LDisplay->parent=this;
    LDisplay->setMouseTracking(false);
    LDisplay->setScaledContents(false);
    LDisplay->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    LDisplay->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    tBarW=new QWidget;
    layoutTBarW= new QVBoxLayout;
    tBarW->setLayout(layoutTBarW);

    layout->addWidget(LDisplay);
    layout->addWidget(tBarW);

    selDisp=new smp_selector("Display mode:", 0, {"Camera","Depth Map","Depth Map SD"});
    layoutTBarW->addWidget(selDisp);

    TWCtrl=new QTabWidget;
    TWCtrl->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
    layoutTBarW->addWidget(TWCtrl);

    pgSGUI=new pgScanGUI(MLP);
    scanRes=pgSGUI->result.getClient();
    pgMGUI=new pgMoveGUI;
    pgTGUI=new pgTiltGUI;
    pgFGUI=new pgFocusGUI(MLP, pgSGUI);
    pgPRGUI=new pgPosRepGUI;
    cm_sel=new smp_selector("tab_camera_smp_selector", "Select colormap: ", 0, OCV_CM::qslabels());
    cMap=new colorMap(cm_sel, exclColor, pgSGUI, pgTGUI);
    tCG=new tabCamGnuplot;
    pgSGUI->pgMGUI=pgMGUI;
    camSet=new cameraSett(pgSGUI->getExpMinMax); connect(pgSGUI, SIGNAL(doneExpMinmax(int,int)), camSet, SLOT(doneExpMinmax(int,int)));

    pgBeAn=new pgBeamAnalysis(MLP, pgMGUI, pgSGUI, camSet);
    pgBGUI=new pgBoundsGUI(pgMGUI,pgBeAn);
    pgDpEv=new pgDepthEval(pgBGUI);
    pgCal=new pgCalib(pgSGUI, pgBGUI, pgFGUI, pgMGUI, pgDpEv, pgBeAn);
    pgWrt=new pgWrite;

    redLaserOn=new QCheckBox("Red Laser");
    redLaserOn->setToolTip("This laser is turned on automatically when autocalibrating. You can hovewer turn it on for some manual operations.");
    connect(redLaserOn, SIGNAL(toggled(bool)), this, SLOT(onRedLaserToggle(bool)));
    greenLaserOn=new QCheckBox("Green Laser");
    greenLaserOn->setToolTip("You have to activate this to turn on the writing laser before any writing can be done. Turned off automatically when closed.");
    connect(greenLaserOn, SIGNAL(toggled(bool)), this, SLOT(onGreenLaserToggle(bool)));

    addInfo=new QLabel; addInfo->setMargin(10);
    addInfo->setVisible(false);

    pageMotion=new twd_selector;
        pageMotion->addWidget(addInfo);
        pageMotion->addWidget(pgSGUI->gui_activation);
        pageMotion->addWidget(pgMGUI->gui_activation);  connect(pgPRGUI, SIGNAL(changed(double,double,double,double)), pgMGUI, SLOT(onFZdifChange(double,double,double,double)));
        pageMotion->addWidget(pgTGUI->gui_activation);
        pageMotion->addWidget(pgFGUI->gui_activation);
        pageMotion->addWidget(pgPRGUI);

    pageWriting=new twd_selector;
        pageWriting->addWidget(new twid(greenLaserOn,redLaserOn, false));
        pageWriting->addWidget(pgBeAn->gui_activation);
        pageWriting->addWidget(pgCal->gui_activation);
        pageWriting->addWidget(pgWrt->gui_activation);
        pageWriting->addWidget(pgBGUI);

    pageProcessing=new twd_selector;
        loadRawBtn=new QPushButton("Load measurement"); connect(loadRawBtn, SIGNAL(released()), this, SLOT(onLoadDepthMapRaw()));
        diff2RawBtn=new QPushButton("Load 2 measurements and dif them"); connect(diff2RawBtn, SIGNAL(released()), this, SLOT(onDiff2Raw()));

        pageProcessing->addWidget(new vtwid(new twid(loadRawBtn),new twid(diff2RawBtn),pgSGUI->gui_processing,false, false));
        pageProcessing->addWidget(pgCal->gui_processing);

    pageSettings=new twd_selector("","Select");
    connect(pageSettings, SIGNAL(changed(int)), this, SLOT(on_tab_change(int)));
        pageSettings->addWidget(pgSGUI->gui_settings,"Scan");   index_pgSGUI=0;
        pageSettings->addWidget(pgMGUI->gui_settings,"Move");
        pageSettings->addWidget(pgTGUI->gui_settings,"Tilt");
        pageSettings->addWidget(pgFGUI->gui_settings,"Focus");  index_pgFGUI=3;
        pageSettings->addWidget(cMap,"ColorMap");
        pageSettings->addWidget(camSet,"Camera");               index_camSet=5;
        pageSettings->addWidget(pgDpEv,"Depth Eval");
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
    pgHistGUI=new pgHistogrameGUI(400, 50, cm_sel, exclColor);
    layoutTBarW->addWidget(pgHistGUI);

    main_show_scale=new checkbox_save(false,"tab_camera_main_show_scale","ScaleBar");
    main_show_target=new checkbox_save(false,"tab_camera_main_show_target","Target");
    main_show_bounds=new checkbox_save(false,"tab_camera_main_show_bounds","Write Bounds");
    layoutTBarW->addWidget(new twid(main_show_scale, main_show_target, main_show_bounds));

    measPB=new QProgressBar; measPB->setRange(0,100);
    compPB=new QProgressBar; compPB->setRange(0,100);
    layoutTBarW->addWidget(new twid(new QLabel("Action: "), measPB, new QLabel("Computation: "), compPB));

    timer=new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));

    clickMenu=new QMenu;
    clickMenu->addAction("Save int. and fft. at this pixel on next measurement", this, SLOT(onSavePixData()));
    clickMenu->addAction("Save image to file", this, SLOT(onSaveCameraPicture()));

    clickMenuDepthRight=new QMenu;
    clickMenuDepthRight->addAction("Save DepthMap (wtih border, scalebar and colorbar) to file", this, SLOT(onSaveDepthMap()));
    clickMenuDepthRight->addAction("Save DepthMap (raw, float) to file", this, SLOT(onSaveDepthMapRaw()));
    clickMenuDepthRight->addAction("Save DepthMap (txt, float) to file", this, SLOT(onSaveDepthMapTxt()));
    clickMenuDepthRight->addAction("Plot Rect (Gnuplot)", this, SLOT(onPlotRect()));
    clickMenuDepthRight->addAction("Rotate DepthMap", this, SLOT(onRotateDepthMap()));
    clickMenuDepthRight->addAction("2D FFT", this, SLOT(on2DFFT()));

    clickMenuDepthLeft=new QMenu;
    clickMenuDepthLeft->addAction("Plot Line (Gnuplot)", this, SLOT(onPlotLine()));
    clickMenuDepthLeft->addAction("Save Line (.txt)", this, SLOT(onSaveLine()));
}

tab_camera::~tab_camera(){  //we delete these as they may have cc_save variables which actually save when they get destroyed, otherwise we don't care as the program will close anyway
    delete scanRes;
    delete pgCal;
    delete pgSGUI;
    delete pgBGUI;
    delete pgBeAn;
    delete pgMGUI;
    delete pgTGUI;
    delete pgFGUI;
    delete pgPRGUI;
    onRedLaserToggle(false);
    onGreenLaserToggle(false);
}

void tab_camera::work_fun(){
    if(selDisp->index==0) framequeueDisp->setUserFps(30,5);
    else framequeueDisp->setUserFps(0);
    onDisplay=framequeueDisp->getUserMat();

    if(selDisp->index==0){  // Camera
        LDisplay->isDepth=false;
        if(onDisplay!=nullptr){
            if(main_show_target->isChecked() || main_show_scale->isChecked() || main_show_bounds->isChecked()){
                cv::Mat temp=onDisplay->clone();
                if(main_show_bounds->isChecked()) pgBGUI->drawBound(&temp, pgMGUI->getNmPPx());
                if(main_show_target->isChecked()) cMap->draw_bw_target(&temp, pgBeAn->writeBeamCenterOfsX, pgBeAn->writeBeamCenterOfsY);
                if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp, pgMGUI->getNmPPx());
                pgCal->drawWriteArea(&temp);
                LDisplay->setPixmap(QPixmap::fromImage(QImage(temp.data, temp.cols, temp.rows, temp.step, QImage::Format_Indexed8)));
//                cv::applyColorMap(temp, temp, OCV_CM::ids[cm_sel->index]);
//                cv::cvtColor(temp, temp, cv::COLOR_BGR2RGB);
//                LDisplay->setPixmap(QPixmap::fromImage(QImage(temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888)));
            }
            else LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
            addInfo->setVisible(false);
        }
        if(scanRes->changed() || cm_sel->index!=oldCm || exclColorChanged || pgHistGUI->changed || loadedScanChanged){
            const pgScanGUI::scanRes* res;
            if(scanRes->changed()) loadedOnDisplay=false;
            if(loadedOnDisplay) res=&loadedScan;
            else res=scanRes->get();
            pgHistGUI->updateImg(res);
            oldCm=cm_sel->index;
        }
    }else if(selDisp->index==1 || selDisp->index==2){   // Depth map or SD
        LDisplay->isDepth=true;
        if(pgDpEv->debugChanged || scanRes->changed() || oldIndex!=selDisp->index || cm_sel->index!=oldCm || exclColorChanged || pgHistGUI->changed || cMap->changed || selectingFlag || lastSelectingFlag || loadedScanChanged){
            const pgScanGUI::scanRes* res;
            if(scanRes->changed()) loadedOnDisplay=false;
            if(loadedOnDisplay) res=&loadedScan;
            else res=scanRes->get();

            if(res!=nullptr){
                res=pgDpEv->getDebugImage(res); //if there is no debug image, it returns res so the command does nothing
                double min,max;
                cv::Mat display;
                if(selDisp->index==1 || res->depthSS.empty()){  //show Depth Map
                    pgHistGUI->updateImg(res, &min, &max);
                    cMap->colormappize(&res->depth, &display, &res->mask, min, max, res->XYnmppx, pgHistGUI->ExclOOR);
                }else{                  //show SD
                    cv::divide(res->depthSS,res->avgNum-1,display);
                    double _min,_max; cv::Point ignore;
                    cv::sqrt(display,display);
                    cv::minMaxLoc(display, &_min, &_max, &ignore, &ignore, res->maskN);
                    pgHistGUI->updateImg(res, &min, &max, 0, _max, &display);
                    cMap->colormappize(&display, &display, &res->mask, 0, max, res->XYnmppx, pgHistGUI->ExclOOR, false, "SD (nm)");
                }
                dispDepthMatRows=res->depth.rows;
                dispDepthMatCols=res->depth.cols;

                if(selectingFlag){
                    if(selectingFlagIsLine) cv::line(display, {selStartX+1,selStartY+(display.rows-res->depth.rows)/2},{selCurX+1,selCurY+(display.rows-res->depth.rows)/2}, cv::Scalar{exclColor.val[2],exclColor.val[1],exclColor.val[0],255}, 1, cv::LINE_AA);
                    else cv::rectangle(display, {selStartX+1,selStartY+(display.rows-res->depth.rows)/2},{selCurX+1,selCurY+(display.rows-res->depth.rows)/2}, cv::Scalar{exclColor.val[2],exclColor.val[1],exclColor.val[0],255}, 1);
                }

                LDisplay->setPixmap(QPixmap::fromImage(QImage(display.data, display.cols, display.rows, display.step, QImage::Format_RGBA8888)));
                if(res->tiltCor[0]!=0 || res->tiltCor[1]!=0 || res->avgNum>1){
                    std::string text;
                    if(res->tiltCor[0]!=0 || res->tiltCor[1]!=0) text+=util::toString("Tilt correction was: X: ",res->tiltCor[0],", Y: ",res->tiltCor[1]);
                    if(res->avgNum>1){
                        if(!text.empty()) text+="\n";
                        text+=util::toString("Averaged ",res->avgNum," measurements.");
                    }
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
    exclColorChanged=false;
    loadedScanChanged=false;

    redLaserOn->setEnabled(go.pRPTY->connected);
    greenLaserOn->setEnabled(go.pRPTY->connected);

    measPB->setValue(MLP.progress_meas);
    compPB->setValue(MLP.progress_comp);
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
    exclColorChanged=true;
}

void tab_camera::tab_entered(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;

    framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeueDisp->setUserFps(30,5);

    timer->start(work_call_time);
}
void tab_camera::tab_exited(){
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
    timer->stop();
}



//  iImageDisplay EVENT HANDLING

void iImageDisplay::mouseMoveEvent(QMouseEvent *event){
    int xcoord=event->pos().x()-(width()-pixmap()->width())/2;
    int ycoord=event->pos().y()-(height()-pixmap()->height())/2;
    parent->selCurX=xcoord;
    parent->selCurY=ycoord;
    if(isDepth){
        parent->selCurX-=1;                                 //correct for drawn margins on image
        parent->selCurY-=(pixmap()->height()-parent->dispDepthMatRows)/2;
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
    int xcoord=event->pos().x()-(width()-pixmap()->width())/2;
    int ycoord=event->pos().y()-(height()-pixmap()->height())/2;
    parent->selStartX=xcoord; parent->selCurX=xcoord;
    parent->selStartY=ycoord; parent->selCurY=ycoord;
    if(isDepth){
        parent->selCurX-=1;                                 //correct for drawn margins on image
        parent->selCurY-=(pixmap()->height()-parent->dispDepthMatRows)/2;
        parent->selStartX-=1;
        parent->selStartY-=(pixmap()->height()-parent->dispDepthMatRows)/2;

        if(event->button()==Qt::RightButton){
            parent->selectingFlag=true;
            parent->selectingFlagIsLine=false;
        }else if(event->button()==Qt::LeftButton){
            parent->selectingFlag=true;
            parent->selectingFlagIsLine=true;
        }
    }
    else{}
}
void iImageDisplay::mouseReleaseEvent(QMouseEvent *event){
    int xcoord=event->pos().x()-(width()-pixmap()->width())/2;
    int ycoord=event->pos().y()-(height()-pixmap()->height())/2;
    parent->selEndX=xcoord;
    parent->selEndY=ycoord;

    if(isDepth){
        parent->selEndX-=1;                                 //correct for drawn margins on image
        parent->selEndY-=(pixmap()->height()-parent->dispDepthMatRows)/2;

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
        if(xcoord<0 || xcoord>=pixmap()->width() || ycoord<0 || ycoord>=pixmap()->height()) return; //ignore events outside pixmap;
        if(event->button()==Qt::LeftButton){
            double DX, DY;
            DX=(pixmap()->width()/2+parent->pgBeAn->writeBeamCenterOfsX-xcoord)*parent->pgMGUI->getNmPPx()/1000000;     //we also correct for real writing beam offset from center
            DY=(pixmap()->height()/2+parent->pgBeAn->writeBeamCenterOfsY-ycoord)*parent->pgMGUI->getNmPPx()/1000000;
            parent->pgMGUI->move(DX,DY,0,0,true);
            if(parent->pgMGUI->reqstNextClickPixDiff) parent->pgMGUI->delvrNextClickPixDiff(pixmap()->width()/2-xcoord, pixmap()->height()/2-ycoord);
        }else if(event->button()==Qt::RightButton){
            parent->clickMenu->popup(QCursor::pos());
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
    if(main_show_bounds->isChecked()) pgBGUI->drawBound(&temp, pgMGUI->getNmPPx());
    if(main_show_target->isChecked()) cMap->draw_bw_target(&temp, pgBeAn->writeBeamCenterOfsX, pgBeAn->writeBeamCenterOfsY);
    if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp, pgMGUI->getNmPPx());
    imwrite(fileName, temp,{cv::IMWRITE_PNG_COMPRESSION,9});
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(fq);
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
        int width=abs(selEndX-selStartX);
        int height=abs(selEndY-selStartY);
        if(selDisp->index==1 || res->depthSS.empty()){
            pgHistGUI->updateImg(res, &min, &max);
            if(width>1 && height>1){
                cv::Mat temp0(res->depth,{selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height});
                cv::Mat temp1(res->mask ,{selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height});
                cv::Mat temp2; bitwise_not(temp1, temp2);
                double minr, maxr;
                cv::Point ignore;
                cv::minMaxLoc(temp0, &minr, &maxr, &ignore, &ignore, temp2);
                cMap->colormappize(&temp0, &display, &temp1, std::min(std::max(min,minr), std::min(max,maxr)), std::max(std::min(max,maxr),std::max(min,minr)), res->XYnmppx, pgHistGUI->ExclOOR, true);
            }
            else cMap->colormappize(&res->depth, &display, &res->mask, min, max, res->XYnmppx, pgHistGUI->ExclOOR, !*cMap->exportSet4WholeVal);
        }else{
            cv::divide(res->depthSS,res->avgNum-1,display);
            double _min,_max; cv::Point ignore;
            cv::sqrt(display,display);
            cv::minMaxLoc(display, &_min, &_max, &ignore, &ignore, res->maskN);
            pgHistGUI->updateImg(res, &min, &max, 0, _max, &display);
            if(width>1 && height>1){
                cv::Mat temp0(display,{selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height});
                cv::Mat temp1(res->mask ,{selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height});
                cv::Mat temp2; bitwise_not(temp1, temp2);
                double minr, maxr;
                cv::Point ignore;
                cv::minMaxLoc(temp0, &minr, &maxr, &ignore, &ignore, temp2);
                cMap->colormappize(&temp0, &display, &temp1, 0, maxr, res->XYnmppx, pgHistGUI->ExclOOR, true, "SD (nm)");
            }
            else cMap->colormappize(&display, &display, &res->mask, 0, max, res->XYnmppx, pgHistGUI->ExclOOR, !*cMap->exportSet4WholeVal, "SD (nm)");
        }
        cv::cvtColor(display, display, cv::COLOR_RGBA2BGRA);
        imwrite(fileName, display,{cv::IMWRITE_PNG_COMPRESSION,9});
    }
}
void tab_camera::onSaveDepthMapRaw(bool txt){
    int width=abs(selEndX-selStartX);
    int height=abs(selEndY-selStartY);
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
        loadedScanChanged=true;
        loadedOnDisplay=true;
    }
}
void tab_camera::onDiff2Raw(){
    pgScanGUI::scanRes scanBefore, scanAfter;
    if(pgScanGUI::loadScan(&scanBefore) && pgScanGUI::loadScan(&scanAfter)){
        pgScanGUI::scanRes scanDif=pgSGUI->difScans(&scanBefore, &scanAfter);
        if(scanDif.depth.empty()) return;
        loadedScan=scanDif;
        loadedScanChanged=true;
        loadedOnDisplay=true;
    }
}
void tab_camera::onRotateDepthMap(){
    double angle=QInputDialog::getDouble(this, "Rotate Depth Map", "Rotation Angle (degrees):", 0, -360, 360, 2);
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    cv::Point2f center((selEndX+selStartX)/2, (selEndY+selStartY)/2);
    loadedScan.depth.setTo(loadedScan.min,loadedScan.mask);              // this is to get rid of infinite edges that are missed by the mask
    cv::Mat TM=cv::getRotationMatrix2D(center, angle, 1.0);
    cv::warpAffine(loadedScan.maskN, loadedScan.maskN, TM, loadedScan.maskN.size());        // this first so that the rest gets filled with zeros, ie bad pixels
    cv::compare(loadedScan.maskN, cv::Scalar(255), loadedScan.mask, cv::CMP_LT);            // spread the nask
    bitwise_not(loadedScan.mask, loadedScan.maskN);
    cv::warpAffine(loadedScan.depth, loadedScan.depth, TM, loadedScan.depth.size());
    loadedScan.depth.setTo(std::numeric_limits<float>::max(),loadedScan.mask);
    if(!loadedScan.depthSS.empty()){
        cv::warpAffine(loadedScan.depthSS, loadedScan.depthSS, TM, loadedScan.depthSS.size());
        loadedScan.depth.setTo(std::numeric_limits<float>::max(),loadedScan.mask);
    }
    cv::Point ignore;
    cv::minMaxLoc(loadedScan.depth, &loadedScan.min, &loadedScan.max, &ignore, &ignore, loadedScan.maskN);
    loadedScanChanged=true;
    loadedOnDisplay=true;
}

void tab_camera::onPlotLine(){
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    tCG->plotLine(res,cv::Point(selStartX,selStartY),cv::Point(selEndX,selEndY),selDisp->index==2);
}
void tab_camera::onSaveLine(){
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    tCG->saveLine(res,cv::Point(selStartX,selStartY),cv::Point(selEndX,selEndY),selDisp->index==2);
}

void tab_camera::onPlotRect(){
    int width=abs(selEndX-selStartX);
    int height=abs(selEndY-selStartY);
    const pgScanGUI::scanRes* res;
    if(loadedOnDisplay) res=&loadedScan;
    else res=scanRes->get();
    if(res==nullptr) return;
    tCG->plotRoi(res, cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height),selDisp->index==2);
}

void tab_camera::on2DFFT(){
    if(!loadedScan.depth.empty() && loadedOnDisplay);
    else if(scanRes->get()!=nullptr) loadedScan=*scanRes->get();
    else return;

    if(selEndX!=selStartX && selEndY!=selStartY){
        int width=abs(selEndX-selStartX);
        int height=abs(selEndY-selStartY);
        loadedScan.depth=loadedScan.depth(cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
        loadedScan.mask =loadedScan.mask (cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
        loadedScan.maskN=loadedScan.maskN(cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
        if(!loadedScan.depthSS.empty())
            loadedScan.depthSS=loadedScan.depthSS(cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
    }

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

    loadedScanChanged=true;
    loadedOnDisplay=true;
}


void tab_camera::onRedLaserToggle(bool state){
    if(!go.pRPTY->connected) return;
    std::vector<uint32_t> commands;
    commands.push_back(CQF::GPIO_MASK(0x80,0,0x00));
    commands.push_back(CQF::GPIO_DIR (0x00,0,0x00));
    commands.push_back(CQF::GPIO_VAL (state?0x80:0x0,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
}
void tab_camera::onGreenLaserToggle(bool state){
    if(!go.pRPTY->connected) return;
    std::vector<uint32_t> commands;
    commands.push_back(CQF::GPIO_MASK(0x20,0,0x00));
    commands.push_back(CQF::GPIO_DIR (0x00,0,0x00));
    commands.push_back(CQF::GPIO_VAL (state?0x20:0x0,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
}
