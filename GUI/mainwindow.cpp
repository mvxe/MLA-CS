#include "GUI/mainwindow.h"
#include "ui_mainwindow.h"
#include "includes.h"

MainWindow::MainWindow(QApplication* qapp, QWidget *parent) : qapp(qapp), QMainWindow(parent), ui(new Ui::MainWindow) {
    connect(qapp,SIGNAL(aboutToQuit()),this,SLOT(program_exit()));
    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q),this,SLOT(program_exit()));

    ui->setupUi(this);
    sync_settings();
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(GUI_update()));
    timer->start(10);

    menu = new QMenu(this);
    menu->setToolTipsVisible(true);
    ui->cam1_select->setMenu(menu);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(cam1_select_show()));

    iuScope_img=go.pMAKO->iuScope->FQsPCcam.getNewFQ();    //make new image queue
    ui->camera_stream->pmw=this;
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::program_exit(){
    timer->stop();
    go.cleanup();
}

void MainWindow::on_tabWidget_currentChanged(int index){
    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch
    if (ui->tabWidget->tabText(index)=="Camera") iuScope_img->setUserFps(30.,50);
    else iuScope_img->setUserFps(0.);
}


void MainWindow::cam1_select_show(){   //on_cam1_select pressed, should update usb device list
    updateCamMenu();
}
void MainWindow::on_cam1_select_triggered(QAction *arg1){   //on_cam1_select action selected
    go.pMAKO->iuScope->ID.set(arg1->text().toStdString());
    ui->cam1_select->setText("camera ID: "+arg1->text());
    go.pMAKO->MAKO_reco=true;
}

void MainWindow::updateCamMenu(){
    menu->clear();
    while (!actptrs.empty()){   //free old menus
        delete actptrs.back();
        actptrs.pop_back();
    }
    for (int i=0;i!=go.pMAKO->cam_desc.get()->size();i++){
        QAction *actx = new QAction(this);
        actx->setText(QString::fromStdString(go.pMAKO->cam_desc.get()->at(i).ID));
        actx->setToolTip(QString::fromStdString(go.pMAKO->cam_desc.get()->at(i).description));
        menu->addAction(actx);
        actptrs.push_back(actx);
    }
}

void mtlabel::mousePressEvent(QMouseEvent *event){
    double disp_x=-(event->pos().x()-size().width()/2.)/size().width();
    double disp_y=-(event->pos().y()-size().height()/2.)/size().height();
    if(go.pXPS->connected)
        go.pXPS->MoveRelative(XPS::mgroup_XYZ,disp_x*pmw->xps_x_sen/100,disp_y*pmw->xps_y_sen/100,0);
}

void mtlabel::wheelEvent(QWheelEvent *event){
    if(go.pXPS->connected)
        go.pXPS->MoveRelative(XPS::mgroup_XYZ,0,0,(double)event->delta()*pmw->xps_z_sen/1000000);
}

void MainWindow::on_btm_kill_released(){        //TODO: this is just a diable timer test, remove
    //disable.set(true,5);
//    for (int i=0;i!=8;i++){
//        exec_ret ret;
//        go.pXPS->execCommand(&ret, "HardwareDriverAndStageGet", i,  "char *");
//        ret.block_till_done();
//        std::cerr<<ret.v.retstr<<"\n";
//    }
//    0,XPS-DRV02;XMS50,EndOfAPI
//    0,XPS-DRV02;XMS100,EndOfAPI
//    0,XPS-DRV03;VP-25XL,EndOfAPI

    //if (lol) {go.pXPS->execCommand(&ret, "GPIODigitalSet","GPIO3.DO", 1,1);lol=false;}
    //else  {go.pXPS->execCommand(&ret, "GPIODigitalSet","GPIO3.DO", 1,0);lol=true;}
    //ret.block_till_done(); std::cerr<<ret.v.retstr<<"\n";
    go.newThread<PFindFocus>(1, 0.01, 50);
}

void MainWindow::on_btn_home_released(){        //TODO: this is a test, remove
    go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,1);

    //go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", "Always", 0, 0, 0, 0, util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".Z.SGamma.ConstantVelocityStart"),0,0,0,0);
    go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.ElementNumberStart"),3,0,0,0);
    //go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.TrajectoryStart"),0,0,0,0);
    go.pXPS->execCommand("EventExtendedConfigurationActionSet", "GPIO3.DO.DOSet",1,0,0,0);
    go.pXPS->execCommand("EventExtendedStart","int *");

    //go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", "Always", 0, 0, 0, 0, util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".Z.SGamma.ConstantVelocityEnd"),0,0,0,0);
    go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.ElementNumberStart"),4,0,0,0);
    //go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.TrajectoryEnd"),0,0,0,0);
    go.pXPS->execCommand("EventExtendedConfigurationActionSet", "GPIO3.DO.DOSet",1,1,0,0);
    go.pXPS->execCommand("EventExtendedStart","int *");


    pPVTobj po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "test666.txt");
    po->add(1,1,1,1,1,1,1);
    po->add(1,0,0,0,0,0,0);
    po->add(1,0,0,0,0,0,0);
    po->add(1,-1,-1,-1,-1,-1,-1);
    po->add(1,0,0,0,0,0,0);
    std::cout<<go.pXPS->verifyPVTobj(po).retstr<<"\n";
    std::cout<<go.pXPS->execPVTobjB(po).retstr<<"\n";
    go.pXPS->destroyPVTobj(po);

    go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,0);
}
