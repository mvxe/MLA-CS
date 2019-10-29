#include "focus.h"
#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"


//GUI

pgFocusGUI::pgFocusGUI(std::mutex &_lock_mes, std::mutex &_lock_comp, pgScanGUI *pgSGUI): _lock_mes(_lock_mes), _lock_comp(_lock_comp), pgSGUI(pgSGUI) {
    PVTScan=go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_PVTfocus.txt").c_str());
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
}

void pgFocusGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QHBoxLayout;
    gui_activation->setLayout(alayout);

}
void pgFocusGUI::onRefocus(){

}

void pgFocusGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    QLabel* tlabel=new QLabel("Some settings borrowed from Scan!");
    slayout->addWidget(tlabel);
    range=new val_selector(10., "pgFocusGUI_range", "Scan Range:", 1., 2000., 3, 3 , {"nm","um",QChar(0x03BB),"L"});
    connect(range, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(range);
    ppwl=new val_selector(1., "pgFocusGUI_ppwl", "Points Per Wavelength: ", 0.001, 100., 3);
    connect(ppwl, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(ppwl);
    calcL=new QLabel;
    slayout->addWidget(calcL);
}

void pgFocusGUI::recalculate() {
    if(_lock_mes.try_lock()){
        if(_lock_comp.try_lock()){
            std::string report;
            updatePVT(report);
            calcL->setText(report.c_str());
            _lock_comp.unlock();
        }
        else timer->start();
        _lock_mes.unlock();
    }
    else timer->start();
}

double pgFocusGUI::vsConv(val_selector* vs){return pgSGUI->vsConv(vs);}
void pgFocusGUI::updatePVT(std::string &report){
    double minFPS,maxFPS;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &maxFPS);
    double readTime=vsConv(range)*vsConv(ppwl)/vsConv(pgSGUI->led_wl)/maxFPS*2;   //s
    double readRangeDis=vsConv(range)/1000000;
    double readVelocity=readRangeDis/readTime;                          //mm/s
    report+=util::toString("Read Time =",readTime," s\nRead Range Distance:",readRangeDis," mm\nRead Velocity:",readVelocity," m/s\n");
    if(readVelocity>vsConv(pgSGUI->max_vel)) {report+="Error: Read Velocity is higher than set max Velocity!\n"; return;}
    double readAccelTime=readVelocity/vsConv(pgSGUI->max_acc);
    double readAccelDis=vsConv(pgSGUI->max_acc)/2*readAccelTime*readAccelTime;
    double Offset=readRangeDis/2+readAccelDis;
    PVTsRdy=false;
    PVTScan->clear();

    double movTime=2*sqrt(2*Offset/vsConv(pgSGUI->max_acc));
    double movMaxVelocity=vsConv(pgSGUI->max_acc)*movTime;

    if(movMaxVelocity > vsConv(pgSGUI->max_vel)){report+="Error: Max move Velocity is higher than set max Velocity (did not implement workaround it cus it was unlikely)!\n"; return;}

    double darkFrameTime=pgSGUI->darkFrameNum/maxFPS;
    totalFrameNum=readTime*maxFPS;
    report+=util::toString("\nTotal expected number of useful frames for focusing: ",totalFrameNum,"\n");
    report+=util::toString("\nTotal time needed for focusing (+computation): ",2*movTime+2*readAccelTime+darkFrameTime+readTime,"\n");

    PVTScan->addAction(XPS::iuScopeLED,true);
    PVTScan->add(movTime, 0,0, 0,0, -Offset,0, 0,0);
    PVTScan->addAction(XPS::iuScopeLED,true);
    PVTScan->add(readAccelTime,  0,0, 0,0, readAccelDis,readVelocity, 0,0);
    PVTScan->add(readTime, 0,0, 0,0, readRangeDis,readVelocity, 0,0);
    PVTScan->addAction(XPS::iuScopeLED,false);
    PVTScan->add(readAccelTime,  0,0, 0,0, readAccelDis,0, 0,0);
    PVTScan->add(darkFrameTime,  0,0, 0,0, 0,0, 0,0);
    PVTScan->addAction(XPS::iuScopeLED,true);
    PVTScan->add(movTime, 0,0, 0,0, -Offset,0, 0,0);

    exec_dat ret;
    ret=go.pXPS->verifyPVTobj(PVTScan);
    if(ret.retval!=0) {report+=util::toString("Error: Verify PVTScan failed, retval was",ret.retstr,"\n"); return;}

    PVTsRdy=true;
}
