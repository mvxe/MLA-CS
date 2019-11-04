#include "tab_camera.h"
#include "GUI/mainwindow.h"
#include "includes.h"

tab_camera::tab_camera(QWidget* parent){
    layout=new QHBoxLayout;
    parent->setLayout(layout);

    LDisplay = new QLabel;
    LDisplay->setMouseTracking(false);
    LDisplay->setFrameShape(QFrame::Box);
    LDisplay->setFrameShadow(QFrame::Plain);
    LDisplay->setScaledContents(false);
    LDisplay->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    tBarW=new QWidget;
    layoutTBarW= new QVBoxLayout;
    tBarW->setLayout(layoutTBarW);

    layout->addWidget(LDisplay);
    layout->addWidget(tBarW);

    selDisp=new smp_selector("Display mode:", 0, {"Camera","Depth Map"});
    layoutTBarW->addWidget(selDisp);

    TWCtrl = new QTabWidget;
    TWCtrl->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
    layoutTBarW->addWidget(TWCtrl);

    pgSGUI=new pgScanGUI(_lock_mes, _lock_comp);
    pgMGUI=new pgMoveGUI;
    pgTGUI=new pgTiltGUI;
    pgFGUI=new pgFocusGUI(_lock_mes, _lock_comp, pgSGUI, pgTGUI);
    pgPRGUI=new pgPosRepGUI;

    pageMotion = new twd_selector(false);
        pageMotion->addWidget(pgSGUI->gui_activation);
        pageMotion->addWidget(pgMGUI->gui_activation);  connect(pgPRGUI, SIGNAL(changed(double,double,double,double)), pgMGUI, SLOT(onFZdifChange(double,double,double,double)));
        pageMotion->addWidget(pgTGUI->gui_activation);
        pageMotion->addWidget(pgFGUI->gui_activation);
        pageMotion->addWidget(pgPRGUI);

    pageWriting = new twd_selector(false);
        //pageWriting->addWidget(pgPRGUI->gui,pgPRGUI->timer);

    pageSettings = new twd_selector;
    connect(pageSettings, SIGNAL(changed(int)), this, SLOT(on_tab_change(int)));
        pageSettings->addWidget(pgSGUI->gui_settings,"Scan");
        pageSettings->addWidget(pgMGUI->gui_settings,"Move");
        pageSettings->addWidget(pgTGUI->gui_settings,"Tilt");
        pageSettings->addWidget(pgFGUI->gui_settings,"Focus");

    TWCtrl->addTab(pageMotion,"Motion");
        //scanOne->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

    TWCtrl->addTab(pageWriting,"Writing");

    TWCtrl->addTab(pageSettings,"Settings");

    cm_sel=new smp_selector("tab_camera_smp_selector", "Select colormap: ", 0, OCV_CM::qslabels());
    layoutTBarW->addWidget(cm_sel);
    epc_sel=new QPushButton("Excl."); epc_sel->setFlat(true); epc_sel->setIcon(QPixmap(":/color.svg"));
    connect(epc_sel, SIGNAL(released()), this, SLOT(on_EP_sel_released()));
    cm_sel->addWidget(epc_sel);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));

}

void tab_camera::work_fun(){
    if(selDisp->index==0) framequeueDisp->setUserFps(30,5);
    else framequeueDisp->setUserFps(0);
    onDisplay=framequeueDisp->getUserMat();

    if(selDisp->index==0){  // Camera
        if(onDisplay!=nullptr){
            LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
        }
    }else{                  // Depth map
        if(pgSGUI->scanRes.changed || oldIndex!=selDisp->index || cm_sel->index!=oldCm || exclColorChanged){
            double min,max;
            cv::Mat *mat;
            mat=pgSGUI->scanRes.getMat(&min, &max);
            if(mat!=nullptr){
                cv::Mat colorImg;
                mat->convertTo(colorImg, CV_8U, ((1<<8)-1)/(max-min),-min*((1<<8)-1)/(max-min));
                cv::applyColorMap(colorImg, colorImg, OCV_CM::ids[cm_sel->index]);
                colorImg.setTo(cv::Scalar(exclColor[0],exclColor[1],exclColor[2]), *(pgSGUI->mask.getMat()));
                LDisplay->setPixmap(QPixmap::fromImage(QImage(colorImg.data, colorImg.cols, colorImg.rows, colorImg.step, QImage::Format_RGB888)));
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
    exclColor[0]=color.red();
    exclColor[1]=color.green();
    exclColor[2]=color.blue();
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
