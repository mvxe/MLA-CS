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

    tBarW=new QWidget;
    layoutTBarW= new QVBoxLayout;
    tBarW->setLayout(layoutTBarW);

    layout->addWidget(LDisplay);
    layout->addWidget(tBarW);

    selDisp=new smp_selector("Display mode:", 0, {"Camera","Depth Map"});
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
    pgSGUI->pgMGUI=pgMGUI;
    camSet=new cameraSett(pgSGUI->getExpMinMax); connect(pgSGUI, SIGNAL(doneExpMinmax(int,int)), camSet, SLOT(doneExpMinmax(int,int)));

    pgBGUI=new pgBoundsGUI;
    pgDpEv=new pgDepthEval;

    tiltCor=new QLabel; tiltCor->setMargin(10);
    tiltCor->setVisible(false);

    pageMotion=new twd_selector;
        pageMotion->addWidget(tiltCor);
        pageMotion->addWidget(pgSGUI->gui_activation);
        pageMotion->addWidget(pgMGUI->gui_activation);  connect(pgPRGUI, SIGNAL(changed(double,double,double,double)), pgMGUI, SLOT(onFZdifChange(double,double,double,double)));
        pageMotion->addWidget(pgTGUI->gui_activation);
        pageMotion->addWidget(pgFGUI->gui_activation);
        pageMotion->addWidget(pgPRGUI);

    pageWriting=new twd_selector;
        pageWriting->addWidget(pgBGUI);

    pageProcessing=new twd_selector;
        loadRawBtn=new QPushButton("Load measurement"); connect(loadRawBtn, SIGNAL(released()), this, SLOT(onLoadDepthMapRaw()));
        pageProcessing->addWidget(new twid(loadRawBtn, false));

    pageSettings=new twd_selector("","Select");
    connect(pageSettings, SIGNAL(changed(int)), this, SLOT(on_tab_change(int)));
        pageSettings->addWidget(pgSGUI->gui_settings,"Scan");   index_pgSGUI=0;
        pageSettings->addWidget(pgMGUI->gui_settings,"Move");
        pageSettings->addWidget(pgTGUI->gui_settings,"Tilt");
        pageSettings->addWidget(pgFGUI->gui_settings,"Focus");  index_pgFGUI=3;
        pageSettings->addWidget(cMap,"ColorMap");
        pageSettings->addWidget(camSet,"Camera");               index_camSet=5;
        pageSettings->addWidget(pgDpEv,"Depth Eval");

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

    clickMenuDepth=new QMenu;
    clickMenuDepth->addAction("Save DepthMap (wtih border, scalebar and colorbar) to file", this, SLOT(onSaveDepthMap()));
    clickMenuDepth->addAction("Save DepthMap (raw, float) to file", this, SLOT(onSaveDepthMapRaw()));
}

tab_camera::~tab_camera(){  //we delete these as they may have cc_save variables which actually save when they get destroyed, otherwise we don't care as the program will close anyway
    delete scanRes;
    delete pgSGUI;
    delete pgMGUI;
    delete pgTGUI;
    delete pgFGUI;
    delete pgPRGUI;
    delete pgBGUI;
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
                if(main_show_target->isChecked()) cMap->draw_bw_target(&temp);
                if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp, pgMGUI->getNmPPx());
                LDisplay->setPixmap(QPixmap::fromImage(QImage(temp.data, temp.cols, temp.rows, temp.step, QImage::Format_Indexed8)));
//                cv::applyColorMap(temp, temp, OCV_CM::ids[cm_sel->index]);
//                cv::cvtColor(temp, temp, cv::COLOR_BGR2RGB);
//                LDisplay->setPixmap(QPixmap::fromImage(QImage(temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888)));
            }
            else LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
            tiltCor->setVisible(false);
        }
        if(scanRes->changed() || cm_sel->index!=oldCm || exclColorChanged || pgHistGUI->changed || loadedScanChanged){
            const pgScanGUI::scanRes* res;
            if(scanRes->changed()) loadedOnDisplay=false;
            if(loadedOnDisplay) res=&loadedScan;
            else res=scanRes->get();
            pgHistGUI->updateImg(res);
            oldCm=cm_sel->index;
        }
    }else{                  // Depth map
        LDisplay->isDepth=true;
        if(pgDpEv->debugChanged || scanRes->changed() || oldIndex!=selDisp->index || cm_sel->index!=oldCm || exclColorChanged || pgHistGUI->changed || cMap->changed || selectingDRB || lastSelectingDRB || loadedScanChanged){
            const pgScanGUI::scanRes* res;
            if(scanRes->changed()) loadedOnDisplay=false;
            if(loadedOnDisplay) res=&loadedScan;
            else res=scanRes->get();

            if(res!=nullptr){
                res=pgDpEv->getDebugImage(res); //if there is no debug image, it returns res so the command does nothing
                double min,max;
                pgHistGUI->updateImg(res, &min, &max);

                cv::Mat display;
                cMap->colormappize(&res->depth, &display, &res->mask, min, max, res->XYnmppx, pgHistGUI->ExclOOR);
                if(selectingDRB) cv::rectangle(display, {selStartX+1,selStartY+1},{selCurX+1,selCurY+1}, cv::Scalar{exclColor.val[2],exclColor.val[1],exclColor.val[0],255}, (abs(selCurX-selStartX)>=50 && abs(selCurY-selStartY)>=50)?1:-1);
                LDisplay->setPixmap(QPixmap::fromImage(QImage(display.data, display.cols, display.rows, display.step, QImage::Format_RGBA8888)));
                if(res->tiltCor[0]!=0 && res->tiltCor[1]!=0){
                    tiltCor->setText(QString::fromStdString(util::toString("Tilt correction was: X: ",res->tiltCor[0],", Y: ",res->tiltCor[1])));
                    tiltCor->setVisible(true);
                } else tiltCor->setVisible(false);

            }
            oldCm=cm_sel->index;
        }
    }
    oldIndex=selDisp->index;
    if(onDisplay!=nullptr) framequeueDisp->freeUserMat();
    lastSelectingDRB=selectingDRB;
    exclColorChanged=false;
    loadedScanChanged=false;

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

    }
    else{}
}
void iImageDisplay::mousePressEvent(QMouseEvent *event){
    int xcoord=event->pos().x()-(width()-pixmap()->width())/2;
    int ycoord=event->pos().y()-(height()-pixmap()->height())/2;
    parent->selStartX=xcoord; parent->selCurX=xcoord;
    parent->selStartY=ycoord; parent->selCurY=ycoord;
    if(isDepth){
        if(event->button()==Qt::RightButton){
            parent->selectingDRB=true;
        }
    }
    else{}
}
void iImageDisplay::mouseReleaseEvent(QMouseEvent *event){
    int xcoord=event->pos().x()-(width()-pixmap()->width())/2;
    int ycoord=event->pos().y()-(height()-pixmap()->height())/2;
    parent->selEndX=xcoord;
    parent->selEndY=ycoord;

    if(!checkIfInBounds(parent->selStartX, parent->selStartY) || !checkIfInBounds(parent->selEndX, parent->selEndY)) {parent->selectingDRB=false; return;}

    if(isDepth){
        if(event->button()==Qt::RightButton){
            parent->clickMenuDepth->popup(QCursor::pos());
            parent->selectingDRB=false;
        }
    }
    else{
        if(event->button()==Qt::LeftButton){
            double DX, DY;
            DX=(pixmap()->width()/2-xcoord)*parent->pgMGUI->getNmPPx()/1000000;
            DY=(pixmap()->height()/2-ycoord)*parent->pgMGUI->getNmPPx()/1000000;
            parent->pgMGUI->onMove(DX*cos(parent->pgMGUI->getAngCamToXMot())+DY*sin(parent->pgMGUI->getAngCamToXMot()+parent->pgMGUI->getYMotToXMot()),DX*sin(parent->pgMGUI->getAngCamToXMot())+DY*cos(parent->pgMGUI->getAngCamToXMot()+parent->pgMGUI->getYMotToXMot()),0,0);
            if(parent->pgMGUI->reqstNextClickPixDiff) parent->pgMGUI->delvrNextClickPixDiff(pixmap()->width()/2-xcoord, pixmap()->height()/2-ycoord);
        }else if(event->button()==Qt::RightButton){
            parent->clickMenu->popup(QCursor::pos());
        }
    }
}
bool iImageDisplay::checkIfInBounds(int xcoord, int ycoord){
    if(isDepth){
        const pgScanGUI::scanRes* res=parent->scanRes->get();
        if(xcoord<1 || xcoord>=res->depth.cols+1 || ycoord<(pixmap()->height()-res->depth.rows)/2 || ycoord>=res->depth.rows+(pixmap()->height()-res->depth.rows)/2 ) return false; //ignore events outside image;
    }else{
        if(xcoord<0 || xcoord>=pixmap()->width() || ycoord<0 || ycoord>=pixmap()->height()) return false; //ignore events outside pixmap;
    }
    return true;
}
void iImageDisplay::wheelEvent(QWheelEvent *event){
    if(isDepth){}
    else{
        parent->pgMGUI->_onMoveZ(10*event->delta());
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
    if(main_show_target->isChecked()) cMap->draw_bw_target(&temp);
    if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp, pgMGUI->getNmPPx());
    imwrite(fileName, temp,{cv::IMWRITE_PNG_COMPRESSION,9});
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(fq);
}
void tab_camera::onSaveDepthMap(void){
    std::string fileName=QFileDialog::getSaveFileName(this,"Select file for saving Depth Map (wtih border, scalebar and colorbar).", "","Images (*.png)").toStdString();
    if(fileName.empty())return;
    if(fileName.find(".png")==std::string::npos) fileName+=".png";
    double min,max;
    const pgScanGUI::scanRes* res=scanRes->get();
    pgHistGUI->updateImg(res, &min, &max);
    if(res!=nullptr){
        cv::Mat display;
        int width=abs(selEndX-selStartX);
        int height=abs(selEndY-selStartY);
        if(width>=50 && height>=50){
            cv::Mat temp0(res->depth,{selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height});
            cv::Mat temp1(res->mask ,{selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height});
            cMap->colormappize(&temp0, &display, &temp1, min, max, res->XYnmppx, pgHistGUI->ExclOOR, true);
        }
        else cMap->colormappize(&res->depth, &display, &res->mask, min, max, res->XYnmppx, pgHistGUI->ExclOOR);
        cv::cvtColor(display, display, cv::COLOR_RGBA2BGRA);
        imwrite(fileName, display,{cv::IMWRITE_PNG_COMPRESSION,9});
    }
}
void tab_camera::onSaveDepthMapRaw(void){
    int width=abs(selEndX-selStartX);
    int height=abs(selEndY-selStartY);
    if(width>=50 && height>=50){
           pgScanGUI::saveScan(scanRes->get(), cv::Rect(selStartX<selEndX?selStartX:(selStartX-width), selStartY<selEndY?selStartY:(selStartY-height), width, height));
    } else pgScanGUI::saveScan(scanRes->get());
}
void tab_camera::onLoadDepthMapRaw(void){
    if(pgScanGUI::loadScan(&loadedScan)){
        loadedScanChanged=true;
        loadedOnDisplay=true;
    }
}
