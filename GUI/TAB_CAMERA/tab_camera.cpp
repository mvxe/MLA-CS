#include "tab_camera.h"
#include "GUI/mainwindow.h"
#include "includes.h"

tab_camera::tab_camera(QWidget* parent){
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

    pgSGUI=new pgScanGUI(_lock_mes, _lock_comp);
    pgMGUI=new pgMoveGUI;
    pgTGUI=new pgTiltGUI;
    pgFGUI=new pgFocusGUI(_lock_mes, _lock_comp, pgSGUI, pgTGUI);
    pgPRGUI=new pgPosRepGUI;
    cm_sel=new smp_selector("tab_camera_smp_selector", "Select colormap: ", 0, OCV_CM::qslabels());
    cMap=new colorMap(cm_sel, exclColor, pgSGUI, pgTGUI);

    pageMotion=new twd_selector(false);
        pageMotion->addWidget(pgSGUI->gui_activation);
        pageMotion->addWidget(pgMGUI->gui_activation);  connect(pgPRGUI, SIGNAL(changed(double,double,double,double)), pgMGUI, SLOT(onFZdifChange(double,double,double,double)));
        pageMotion->addWidget(pgTGUI->gui_activation);
        pageMotion->addWidget(pgFGUI->gui_activation);
        pageMotion->addWidget(pgPRGUI);

    pageWriting=new twd_selector(false);
        //pageWriting->addWidget(pgPRGUI->gui,pgPRGUI->timer);

    pageSettings=new twd_selector;
    connect(pageSettings, SIGNAL(changed(int)), this, SLOT(on_tab_change(int)));
        pageSettings->addWidget(pgSGUI->gui_settings,"Scan");
        pageSettings->addWidget(pgMGUI->gui_settings,"Move");
        pageSettings->addWidget(pgTGUI->gui_settings,"Tilt");
        pageSettings->addWidget(pgFGUI->gui_settings,"Focus");
        pageSettings->addWidget(cMap,"ColorMap");

    TWCtrl->addTab(pageMotion,"Motion");
        //scanOne->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

    TWCtrl->addTab(pageWriting,"Writing");

    TWCtrl->addTab(pageSettings,"Settings");

    layoutTBarW->addWidget(cm_sel);
    epc_sel=new QPushButton("Excl."); epc_sel->setFlat(true); epc_sel->setIcon(QPixmap(":/color.svg"));
    connect(epc_sel, SIGNAL(released()), this, SLOT(on_EP_sel_released()));
    cm_sel->addWidget(epc_sel);
    pgHistGUI=new pgHistogrameGUI(400, 50, &pgSGUI->scanRes, &pgSGUI->maskN, cm_sel, exclColor);
    layoutTBarW->addWidget(pgHistGUI);

    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    main_show_scale=new checkbox_save(false,"tab_camera_main_show_scale","ScaleBar");
    tlay->addWidget(main_show_scale);
    main_show_target=new checkbox_save(false,"tab_camera_main_show_target","Target");
    tlay->addWidget(main_show_target);
    tlay->addStretch(0); tlay->setMargin(0);
    layoutTBarW->addWidget(twid);

    timer=new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));
}

tab_camera::~tab_camera(){  //we delete these as they may have cc_save variables which actually save when they get destroyed, otherwise we don't care as the program will close anyway
    delete pgSGUI;
    delete pgMGUI;
    delete pgTGUI;
    delete pgFGUI;
    delete pgPRGUI;
}

void tab_camera::work_fun(){
    if(selDisp->index==0) framequeueDisp->setUserFps(30,5);
    else framequeueDisp->setUserFps(0);
    onDisplay=framequeueDisp->getUserMat();

    if(selDisp->index==0){  // Camera
        if(onDisplay!=nullptr){
            if(main_show_target->isChecked() || main_show_scale->isChecked()){
                cv::Mat temp;
                temp=onDisplay->clone();
                if(main_show_target->isChecked()) cMap->draw_bw_target(&temp);
                if(main_show_scale->isChecked()) cMap->draw_bw_scalebar(&temp);
                LDisplay->setPixmap(QPixmap::fromImage(QImage(temp.data, temp.cols, temp.rows, temp.step, QImage::Format_Indexed8)));
            }
            else LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
        }
        if(pgSGUI->scanRes.changed || cm_sel->index!=oldCm || exclColorChanged || pgHistGUI->changed){
            pgHistGUI->updateImg();
            oldCm=cm_sel->index;
            exclColorChanged=false;
        }
    }else{                  // Depth map
        if(pgSGUI->scanRes.changed || oldIndex!=selDisp->index || cm_sel->index!=oldCm || exclColorChanged || pgHistGUI->changed || cMap->changed){
            double min,max;
            pgHistGUI->updateImg(&min, &max);
            cv::Mat* mat;
            mat=pgSGUI->scanRes.getMat();
            if(mat!=nullptr){
                cv::Mat display;
                cMap->colormappize(mat, &display, pgSGUI->mask.getMat(), min, max, pgHistGUI->ExclOOR);
                QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGBA8888);
                LDisplay->setPixmap(QPixmap::fromImage(qimg));
            }
            oldCm=cm_sel->index;
            exclColorChanged=false;
        }
    }
    oldIndex=selDisp->index;
    if(onDisplay!=nullptr) framequeueDisp->freeUserMat();
}

void tab_camera::on_tab_change(int index){
    if(index==index_pgSGUI) Q_EMIT pgSGUI->recalculate();
    else if(index==index_pgFGUI) Q_EMIT pgFGUI->recalculate();
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

}
void iImageDisplay::mousePressEvent(QMouseEvent *event){

}
void iImageDisplay::mouseReleaseEvent(QMouseEvent *event){
    if((event->button()==Qt::LeftButton)){
        int disX=size().width()/2-event->pos().x()+1;
        int disY=size().height()/2-event->pos().y()+2;
        parent->pgMGUI->onMove(disX*parent->cMap->getXYnmppx()/1000000,disY*parent->cMap->getXYnmppx()/1000000,0,0);
    }
}
void iImageDisplay::wheelEvent(QWheelEvent *event){

}
