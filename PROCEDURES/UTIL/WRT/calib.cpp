#include "calib.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/focus.h"
#include "PROCEDURES/UTIL/USC/move.h"
#include "PROCEDURES/UTIL/WRT/bounds.h"


pgCalib::pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv): pgSGUI(pgSGUI), pgBGUI(pgBGUI), pgFGUI(pgFGUI), pgMGUI(pgMGUI), pgDpEv(pgDpEv){
    gui_activation=new QWidget;
    alayout=new QHBoxLayout;
    gui_activation->setLayout(alayout);
    btnGoToNearestFree=new QPushButton("Go to nearest free");
    btnGoToNearestFree->setToolTip("This adheres to write boundaries!");
    connect(btnGoToNearestFree, SIGNAL(released()), this, SLOT(goToNearestFree()));
    hcGoToNearestFree=new hidCon(btnGoToNearestFree);
    alayout->addWidget(hcGoToNearestFree);
    selRadDilGoToNearestFree=new val_selector(1, "pgCalib_selRadDilGoToNearestFree", "Exclusion Dilation Radius: ", 0, 100, 2, 0, {"um"});
    selRadSprGoToNearestFree=new val_selector(1, "pgCalib_selRadSprGoToNearestFree", "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
    hcGoToNearestFree->addWidget(selRadDilGoToNearestFree);
    hcGoToNearestFree->addWidget(selRadSprGoToNearestFree);

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    scanRes=pgSGUI->result.getClient();
}
pgCalib::~pgCalib(){
    delete scanRes;
}


bool pgCalib::goToNearestFree(){
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //if there is a measurement in progress, wait till its done
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);       //first check if the current scan result is valid (done oncurrent position)
    bool redoScan=false;
    const pgScanGUI::scanRes* res=scanRes->get();
    if(res!=nullptr){
        for(int i=0;i!=3;i++) if(res->pos[i]!=tmp.pos[i])redoScan=true;
    }else redoScan=true;
    if(redoScan){
        pgSGUI->doOneRound();
        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        res=scanRes->get();
    }
    int dil=(selRadDilGoToNearestFree->val*1000/pgMGUI->getNmPPx()-0.5); if(dil<0) dil=0;
    cv::Mat mask=pgDpEv->getMaskFlatness(res, pgMGUI->getNmPPx(), dil);
    int ptX,ptY;
    imgAux::getNearestFreePointToCenter(&mask, ptX, ptY, selRadSprGoToNearestFree->val);    //make radius choosable in nm
    if(ptX==-1){
        std::cerr<<"No free nearby!\n";
        return true;
    }

    double dXumm, dYumm, dXmm, dYmm;
    dXumm=(ptX-res->depth.cols/2)*pgMGUI->getNmPPx()/1000000;
    dYumm=(ptY-res->depth.rows/2)*pgMGUI->getNmPPx()/1000000;
    dXmm=dXumm*cos(pgMGUI->getAngCamToXMot())+dYumm*sin(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());
    dYmm=dXumm*sin(pgMGUI->getAngCamToXMot())+dYumm*cos(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());

    pgMGUI->onMove(-dXmm,-dYmm,0,0);
    return false;
}
