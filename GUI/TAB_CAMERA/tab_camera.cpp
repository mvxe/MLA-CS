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

    selDisp=new smp_selector("Display mode:", 0, {"Camera","FFT","FFT+overlay","FFT-UnWr" });
    layoutTBarW->addWidget(selDisp);

    TWCtrl = new QTabWidget;
    TWCtrl->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
    layoutTBarW->addWidget(TWCtrl);

    pgSGUI=new pgScanGUI(_lock_mes, _lock_comp);
    pgMGUI=new pgMoveGUI;
    pgTGUI=new pgTiltGUI;
    pgFGUI=new pgFocusGUI(_lock_mes, _lock_comp, pgSGUI);

    pageMotion = new twd_selector(false);
        pageMotion->addWidget(pgSGUI->gui_activation);
        pageMotion->addWidget(pgMGUI->gui_activation);
        pageMotion->addWidget(pgTGUI->gui_activation);
        pageMotion->addWidget(pgFGUI->gui_activation);

    pageWriting = new QWidget;
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

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));

}
tab_camera::~tab_camera(){
    pgSGUI->getCentered();
    while(!pgSGUI->procDone || !pgSGUI->roundDone);
}

void tab_camera::work_fun(){


    if(selDisp->index==0) framequeueDisp->setUserFps(30,5);
    else framequeueDisp->setUserFps(0);
    onDisplay=framequeueDisp->getUserMat();

    cv::Mat *mat;
    switch(selDisp->index){
    case 3: if(pgSGUI->measuredPU.changed || oldIndex!=selDisp->index || cm_sel->index!=oldCm){
                mat=pgSGUI->measuredPU.getMat();                        //FFT Unwrapped
                if(mat!=nullptr){
                    cv::Mat colorImg;
                    cv::applyColorMap(*mat, colorImg, OCV_CM::ids[cm_sel->index]);
                    LDisplay->setPixmap(QPixmap::fromImage(QImage(colorImg.data, colorImg.cols, colorImg.rows, colorImg.step, QImage::Format_RGB888)));
                }
            }
        break;
    case 2:
    case 1: if(pgSGUI->measuredP.changed || oldIndex!=selDisp->index || cm_sel->index!=oldCm){
                mat=pgSGUI->measuredP.getMat();                         //FFT+overlay
                if(mat!=nullptr){
                    cv::Mat colorImg;
                    cv::applyColorMap(*mat, colorImg, OCV_CM::ids[cm_sel->index]);
                    if(selDisp->index==2) colorImg.setTo(cv::Scalar(255,0,0), *pgSGUI->measuredM.getMat());
                    LDisplay->setPixmap(QPixmap::fromImage(QImage(colorImg.data, colorImg.cols, colorImg.rows, colorImg.step, QImage::Format_RGB888)));
                }
            }
        break;
    case 0: if(onDisplay!=nullptr){                         //Camera
                LDisplay->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
            }
        break;
    }
    oldIndex=selDisp->index;
    oldCm=cm_sel->index;

    if(onDisplay!=nullptr) framequeueDisp->freeUserMat();
}

void tab_camera::on_tab_change(int index){
    switch (index){
        case index_pgSGUI:  Q_EMIT pgSGUI->recalculate();
                            break;
        case index_pgFGUI:  Q_EMIT pgFGUI->recalculate();
                            break;
    }
}




void tab_camera::tab_entered(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;

    framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeueDisp->setUserFps(30,5);

    timer->start(work_call_time);
    pageSettings->timerStart();
}
void tab_camera::tab_exited(){
    pgSGUI->getCentered();
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
    timer->stop();
    pageSettings->timerStop();
}
