#include "GUI/mainwindow.h"
#include "ui_mainwindow.h"
#include "includes.h"
#include <opencv2/highgui/highgui.hpp>  // Video write

MainWindow::MainWindow(QApplication* qapp, QWidget *parent) : qapp(qapp), QMainWindow(parent), ui(new Ui::MainWindow) {
    mats= new std::vector<cv::Mat>;

    onDisplay=new cv::Mat();
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

    menu2 = new QMenu(this);
    menu2->setToolTipsVisible(true);
    ui->cam2_select->setMenu(menu2);
    connect(menu2, SIGNAL(aboutToShow()), this, SLOT(cam2_select_show()));

    menu3 = new QMenu(this);
    menu3->setToolTipsVisible(true);
    ui->cnc_select->setMenu(menu3);
    connect(menu3, SIGNAL(aboutToShow()), this, SLOT(cnc_select_show()));

    iuScope_img=go.pGCAM->iuScope->FQsPCcam.getNewFQ();    //make new image queue
    utilCam_img=go.pGCAM->utilCam->FQsPCcam.getNewFQ();    //make new image queue
    ui->camera_stream->pmw=this;
    ui->utilcam_stream->pmw=this;

    tabDev=new tab_devices(ui->tab_dev);
    tabPlot=new tab_temp_plot(ui->tab_dis);
    tabPlot->pmw=this;
}

MainWindow::~MainWindow(){
    delete ui;
    delete onDisplay;
    delete tabDev;
    delete tabPlot;
}

void MainWindow::program_exit(){
    timer->stop();
    go.cleanup();
}

void MainWindow::on_tabWidget_currentChanged(int index){
    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch
    if (ui->tabWidget->tabText(index)=="Camera") iuScope_img->setUserFps(30.,5);
    else iuScope_img->setUserFps(0.);
    if (ui->tabWidget->tabText(index)=="UtilCam") utilCam_img->setUserFps(30.,5);
    else utilCam_img->setUserFps(0.);
}


void MainWindow::cam1_select_show(){   //on_cam1_select pressed, should update usb device list
    updateCamMenu(menu);
}
void MainWindow::on_cam1_select_triggered(QAction *arg1){   //on_cam1_select action selected
    go.pGCAM->iuScope->selected_ID.set(arg1->text().toStdString());
    ui->cam1_select->setText("camera ID: "+arg1->text());
    go.pGCAM->iuScope->checkID=true;
}

void MainWindow::cam2_select_show(){
    updateCamMenu(menu2);
}
void MainWindow::on_cam2_select_triggered(QAction *arg1){
    go.pGCAM->utilCam->selected_ID.set(arg1->text().toStdString());
    ui->cam2_select->setText("camera ID: "+arg1->text());
    go.pGCAM->utilCam->checkID=true;
}

void MainWindow::cnc_select_show(){
    updateCncMenu(menu3);
}
void MainWindow::on_cnc_select_triggered(QAction *arg1){
    go.pCNC->selected_ID.set(arg1->text().toStdString());
    ui->cnc_select->setText("serial ID: "+arg1->text());
    go.pCNC->checkID=true;
}

void MainWindow::updateCamMenu(QMenu* menuN){
    menuN->clear();
    for (int i=0;i!=go.pGCAM->cam_desc.get()->size();i++){
        QAction *actx = new QAction(this);
        actx->setText(QString::fromStdString(go.pGCAM->cam_desc.get()->at(i).ID));
        actx->setToolTip(QString::fromStdString(go.pGCAM->cam_desc.get()->at(i).description));
        menuN->addAction(actx);
    }
}

void MainWindow::updateCncMenu(QMenu* menuN){
    go.pCNC->refreshID=true;
    while(go.pCNC->refreshID) std::this_thread::sleep_for (std::chrono::milliseconds(1));
    menuN->clear();
    for (int i=0;i!=go.pCNC->serial_desc.get()->size();i++){
        QAction *actx = new QAction(this);
        actx->setText(QString::fromStdString(go.pCNC->serial_desc.get()->at(i).ID));
        actx->setToolTip(QString::fromStdString(go.pCNC->serial_desc.get()->at(i).description));
        menuN->addAction(actx);
    }
}

void mtlabel::mousePressEvent(QMouseEvent *event){
    double disp_x=-(event->pos().x()-size().width()/2.)/size().width()*1280;
    double disp_y=-(event->pos().y()-size().height()/2.)/size().height()*1024;
    if(go.pXPS->connected)
        go.pXPS->MoveRelative(XPS::mgroup_XYZ,disp_x*pmw->xps_x_sen/100000,disp_y*pmw->xps_y_sen/100000,0);
}

void mtlabel::wheelEvent(QWheelEvent *event){
    if(go.pXPS->connected)
        go.pXPS->MoveRelative(XPS::mgroup_XYZ,0,0,(double)event->delta()*pmw->xps_z_sen/1000000);
}

void MainWindow::on_btm_kill_released(){
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
    //iuScope_img->setUserFps(0.);
    go.pXPS->killGroups();
//    exec_ret ret;
//    go.pXPS->execCommand(&ret,"PositionerCorrectorAutoTuning",util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".Z"), 1,"double *","double *","double *");
//    ret.block_till_done();
//    std::cerr<<ret.v.retstr<<"\n";
}

void MainWindow::on_btn_home_released(){
    go.pXPS->initGroups();
    go.pXPS->homeGroups();
}

void MainWindow::on_btn_focus_released(){
    go.newThread<PFindFocus>(-999, 1, 0.1, 50, 0);
}

void MainWindow::on_btn_depthdmap_released(){
    QString fileName = QFileDialog::getSaveFileName(this,tr("Image"), "",tr("Images (*.png *.xpm *.jpg)"));
    if(fileName.isEmpty()) return;
    go.newThread<pGetDepthMap>(0.01, -0.00144, 0.001, 50, fileName.toStdString());
}

void MainWindow::on_btn_calXY_released(){
    std::cout<<a<<" "<<xps_x_sen<<" "<<b<<" "<<xps_y_sen<<"\n";
    xps_x_sen=a;
    xps_y_sen=b;
    go.newThread<pCalibrateXY>(0.03, &a, &b);
}

void MainWindow::on_btn_wrtingTest_released(){
    go.newThread<pWritingTest>();
}

void MainWindow::on_btn_save_img_released(){
    const cv::Mat* dmat=nullptr;
    do{
        dmat=iuScope_img->getUserMat();
    } while (dmat==nullptr);
    QString fileName = QFileDialog::getSaveFileName(this,tr("Image"), "",tr("Images (*.png *.xpm *.jpg)"));
    if(fileName.isEmpty()) return;
    std::cout<<"Saving image to "<<fileName.toStdString()<<"\n";
    imwrite(fileName.toStdString(), *dmat);
}

void MainWindow::on_btn_PBurnArray_released(){
    go.newThread<pBurnArray>(ui->sb_PBurnArray_spacing->value(), ui->sb_PBurnArray_dotfst->value(), ui->sb_PBurnArray_dotlst->value(), ui->sb_PBurnArray_xgrid->value(), ui->sb_PBurnArray_ygrid->value(), ui->checkBox->isChecked());
}



void MainWindow::on_pushButton_2_released(){
    go.pCNC->execCommand("G28 X\n");
    ui->doubleSpinBox_2->setValue(0);
}
void MainWindow::on_pushButton_released(){
    go.pCNC->execCommand("M400\n");     //wait for current moves to finish
    go.pCNC->execCommand("M42 P3 S255\n");
    go.pCNC->execCommand("G4 P10\n");   //wait in ms
    go.pCNC->execCommand("M400\n");
    go.pCNC->execCommand("M42 P3 S0\n");
}
void MainWindow::on_doubleSpinBox_2_editingFinished(){
    go.pCNC->execCommand("G0 X",ui->doubleSpinBox_2->value()," F",ui->doubleSpinBox->value(),"\n");
}
void MainWindow::on_checkBox_2_toggled(bool checked){
    if(checked) go.pGCAM->utilCam->set_trigger("Line1");
    else go.pGCAM->utilCam->set_trigger();
}

void MainWindow::on_pushButton_3_released(){
    expsize=(int)((ui->doubleSpinBox_4->value()-ui->doubleSpinBox_3->value())/ui->doubleSpinBox_5->value())+1;
    go.newThread<pProfileBeam>(ui->doubleSpinBox_3->value(),ui->doubleSpinBox_4->value(),ui->doubleSpinBox_5->value(),ui->doubleSpinBox->value(),10,mats, &matlk);
}
void MainWindow::on_pushButton_4_released(){    //save video
    std::lock_guard<std::mutex>lock(matlk);
    if (mats->empty()) return;

    QString fileName = QFileDialog::getSaveFileName(this,tr("video"), "",tr("Videos (*.avi)"));
    if(fileName.isEmpty()) return;
    std::cout<<"Saving video to "<<fileName.toStdString()<<"\n";

    cv::VideoWriter outputVideo;
    outputVideo.open(fileName.toStdString() , cv::VideoWriter::fourcc('H','2','6','4'), ui->spinBox->value() , mats->front().size(), false);
    matsbar=false;
    ui->progressBar->setMaximum(mats->size()-1);
    ui->progressBar->setFormat("Save progress: %p%");
    for(int i=0;i!=mats->size(); i++){
        ui->progressBar->setValue(i);
        outputVideo << mats->at(i);
    }
}
