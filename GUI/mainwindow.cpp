#include "GUI/mainwindow.h"
#include "ui_mainwindow.h"
#include "includes.h"
#include <opencv2/highgui/highgui.hpp>  // Video write
#include "UTIL/pipe.h"                  //gnuplot

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
    ui->focusbtn->pmw=this;

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
        go.pXPS->MoveRelative(XPS::mgroup_XYZF,disp_x*pmw->xps_x_sen/100000,disp_y*pmw->xps_y_sen/100000,0,0);
}

void mtlabel::wheelEvent(QWheelEvent *event){
    if(go.pXPS->connected)
        go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,(double)event->delta()*pmw->xps_z_sen/1000000,0);
}

void fclabel::wheelEvent(QWheelEvent *event){
    if(go.pXPS->connected)
        go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,0,(double)event->delta()*pmw->xps_f_sen/1000000);
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
    {std::lock_guard<std::mutex>lock(matlk);
    expsize=(int)((ui->doubleSpinBox_4->value()-ui->doubleSpinBox_3->value())/ui->doubleSpinBox_5->value())+1;}
    if (!fitres.empty()) fitres.clear();
    go.newThread<pProfileBeam>(ui->doubleSpinBox_3->value(),ui->doubleSpinBox_4->value(),ui->doubleSpinBox_5->value(),ui->doubleSpinBox->value(),10,mats, &matlk);
    ui->pushButton_5->setText("Fit Frames (64div)"); fitdiv=64;
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
void MainWindow::on_doubleSpinBox_5_valueChanged(double arg1){
    ui->fitstep_val->setValue(arg1);
}
void MainWindow::on_pushButton_6_released(){    //load video
    std::lock_guard<std::mutex>lock(matlk);

    QString fileName = QFileDialog::getOpenFileName(this,tr("video"), "",tr("Videos (*.avi)"));
    if(fileName.isEmpty()) return;
    std::cout<<"Loading video from "<<fileName.toStdString()<<"\n";

    if (!mats->empty()) mats->clear();
    if (!fitres.empty()) fitres.clear();
    cv::VideoCapture inputVideo(fileName.toStdString());
    if (!inputVideo.isOpened()) {std::cout<<"Cannot open\n"; return;}
    cv::Mat src;
    matsbar=false;
    int cframe=2;
    ui->progressBar->setMaximum(cframe);
    ui->progressBar->setFormat("Load progress: %p%");
    for(int i=0;;i++){
        if(i>cframe) {cframe*=2;ui->progressBar->setMaximum(cframe);}
        ui->progressBar->setValue(i);
        inputVideo>>src;
        if (src.empty()) break;
        cv::cvtColor(src, src, CV_BGR2GRAY);
        mats->push_back(src);
    }
    expsize=mats->size();
    ui->progressBar->setMaximum(expsize);
    ui->pushButton_5->setText("Fit Frames (64div)"); fitdiv=64;
}
void MainWindow::on_pushButton_5_released(){    //fit frames
    std::lock_guard<std::mutex>lock(matlk);
    if (mats->empty()) return;

    ui->progressBar->setMaximum(mats->size()-1);
    ui->progressBar->setFormat("Fit progress: %p%");
    ui->progressBar->setValue(0);
    cv::Mat resized;

    pixdim=ui->pixsize->value();
    stepsize=ui->fitstep_val->value();
    fitar cfit;
    cfit.posX[0]=mats->at(0).cols/2;
    cfit.posY[0]=mats->at(0).rows/2;
    cfit.wdtX[0]=cfit.posX[0]/10;
    cfit.wdtY[0]=cfit.posY[0]/10;
    cfit.pow[0]=1000;
    cfit.bgn[0]=1;
    cfit.ang[0]=0.1;

    gnuplot a;   //TODO: apparently gnuplot replies into cerr instead of cout, or our pipe implementation is broken. investigate!
                 //TODO: bug when calling **persist**, the gnuplot process stays open after program close
    a.POUT<<"set terminal wxt enhanced **persist** size 1500,900\n";
    a.POUT<<"set xlabel \"x / um\"\n";
    a.POUT<<"set ylabel \"y / um\"\n";
    a.POUT<<"set zlabel \"I / a.u.\"\n";
    a.POUT<<"set hidden3d front\n";
    for(int i=0;i!=mats->size();i++){
        if(fitres.size()>i){
                cfit=fitres[i];
        }

        a.POUT.flush();
        a.POUT<<"gaussX(x)=2*pow/pi/(wdtX**2)*exp(-2*((x-posX)**2)/(wdtX**2))\n";
        a.POUT<<"gaussY(x)=2*pow/pi/(wdtY**2)*exp(-2*((x-posY)**2)/(wdtY**2))\n";
        a.POUT<<"gaussXY(x,y)=bgn+gaussX(x*cos(ang)+y*sin(ang))*gaussY(x*sin(ang)+y*cos(ang))\n";
        a.POUT<<"wdtX="<<cfit.wdtX[0]/fitdiv<<"\n";
        a.POUT<<"wdtY="<<cfit.wdtY[0]/fitdiv<<"\n";
        a.POUT<<"posX="<<cfit.posX[0]/fitdiv<<"\n";
        a.POUT<<"posY="<<cfit.posY[0]/fitdiv<<"\n";
        a.POUT<<"pow="<<cfit.pow[0]<<"\n";
        a.POUT<<"bgn="<<cfit.bgn[0]<<"\n";
        a.POUT<<"ang="<<cfit.ang[0]<<"\n";
        a.POUT<<"echo = \""<<keyword<<"\"\n";
        a.POUT<<"fit gaussXY(x,y) \"-\" using 1:2:3 via bgn, pow, wdtX, wdtY, posX, posY, ang\n";
        cv::resize(mats->at(i), resized, cv::Size(), 1./fitdiv, 1./fitdiv, cv::INTER_CUBIC);
        for(int x=0;x!=resized.cols;x++){
            for(int y=0;y!=resized.rows;y++){
                a.POUT<< x <<" "<<y<<" "<< (int)resized.at<uint8_t>(y,x)<<"\n";
            }
        }
        a.POUT << "e\n";
        a.POUT<<"print echo\n";
        a.POUT.flush();
        char line[256];
        QString lines;
        while(1){
            a.PERR.getline(line,256);
            //std::cerr<<line;
            lines=line;
            if (lines.contains(keyword, Qt::CaseSensitive)) break;
        }
        a.POUT<<"print abs(wdtX)\n";    a.POUT.flush();
        a.PERR>>cfit.wdtX[0]; cfit.wdtX[0]*=fitdiv;
        a.POUT<<"print abs(wdtY)\n";    a.POUT.flush();
        a.PERR>>cfit.wdtY[0]; cfit.wdtY[0]*=fitdiv;
        a.POUT<<"print posX\n";         a.POUT.flush();
        a.PERR>>cfit.posX[0]; cfit.posX[0]*=fitdiv;
        a.POUT<<"print posY\n";         a.POUT.flush();
        a.PERR>>cfit.posY[0]; cfit.posY[0]*=fitdiv;
        a.POUT<<"print pow\n";          a.POUT.flush();
        a.PERR>>cfit.pow[0];
        a.POUT<<"print bgn\n";          a.POUT.flush();
        a.PERR>>cfit.bgn[0];
        a.POUT<<"print ang\n";          a.POUT.flush();
        a.PERR>>cfit.ang[0];

        a.POUT<<"print wdtX_err\n";     a.POUT.flush();
        a.PERR>>cfit.wdtX[1]; cfit.wdtX[1]*=fitdiv;
        a.POUT<<"print wdtY_err\n";     a.POUT.flush();
        a.PERR>>cfit.wdtY[1]; cfit.wdtY[1]*=fitdiv;
        a.POUT<<"print posX_err\n";     a.POUT.flush();
        a.PERR>>cfit.posX[1]; cfit.posX[1]*=fitdiv;
        a.POUT<<"print posY_err\n";     a.POUT.flush();
        a.PERR>>cfit.posY[1]; cfit.posY[1]*=fitdiv;
        a.POUT<<"print pow_err\n";      a.POUT.flush();
        a.PERR>>cfit.pow[1];
        a.POUT<<"print bgn_err\n";      a.POUT.flush();
        a.PERR>>cfit.bgn[1];
        a.POUT<<"print ang_err\n";      a.POUT.flush();
        a.PERR>>cfit.ang[1];

//        std::cout<<"Image #"<<i<<":\n";
//        std::cout<<"    wdtX="<<cfit.wdtX[0]*pixdim<<" +- "<<cfit.wdtX[1]*pixdim<<"\n";
//        std::cout<<"    wdtY="<<cfit.wdtY[0]*pixdim<<" +- "<<cfit.wdtY[1]*pixdim<<"\n";
//        std::cout<<"    posX="<<cfit.posX[0]*pixdim<<" +- "<<cfit.posX[1]*pixdim<<"\n";
//        std::cout<<"    posY="<<cfit.posY[0]*pixdim<<" +- "<<cfit.posY[1]*pixdim<<"\n";
//        std::cout<<"    pow="<<cfit.pow[0]<<" +- "<<cfit.pow[1]<<"\n";
//        std::cout<<"    bgn="<<cfit.bgn[0]<<" +- "<<cfit.bgn[1]<<"\n";
//        std::cout<<"    ang="<<cfit.ang[0]<<" +- "<<cfit.ang[1]<<"\n";

        a.POUT<<"set isosamples 50\n";
        a.POUT<<"set view equal xy\n";
        a.POUT<<"set title \"frame "<<i+1<<"/"<<mats->size()<<"\"\n";
        a.POUT<<"scaled(x,y)=gaussXY(x/"<<fitdiv*pixdim<<",y/"<<fitdiv*pixdim<<")\n";
        a.POUT<<"splot \"-\" using 1:2:3 w p pt 7 lc 6 title \"raw data(div64)\", scaled(x,y) w l lc 4 title \"fit\"\n";
        cv::resize(mats->at(i), resized, cv::Size(), 1./64, 1./64, cv::INTER_CUBIC);
        for(int x=0;x!=resized.cols;x++){
            for(int y=0;y!=resized.rows;y++){
                a.POUT<<x*64*pixdim<<" "<<y*64*pixdim<<" "<< (int)resized.at<uint8_t>(y,x)<<"\n";
            }
            a.POUT<<"\n";
        }
        a.POUT << "e\n";
        a.POUT.flush();
        ui->progressBar->setValue(i);

        if(fitres.size()>i)
            fitres[i]=cfit;
        else fitres.emplace_back(cfit);
    }


    if(fitdiv==1) return;
    fitdiv/=2;
    ui->pushButton_5->setText(QString::fromStdString(util::toString("Fit Frames (",fitdiv,"div)")));

//    int num=0;
//    std::ofstream myfile;
//    myfile.open ("example.txt");
//    cv::resize(mats->at(num), mats->at(num), cv::Size(), 1./64, 1./64, cv::INTER_CUBIC);
//    for(int x=0;x!=mats->at(num).cols;x++){
//        for(int y=0;y!=mats->at(num).rows;y++){
//            myfile << x <<" "<<y<<" "<< (int)mats->at(num).at<uint8_t>(y,x)<<"\n";
//        }
//        myfile << "e\n";
//    }
//    myfile.close();
}

void MainWindow::on_pushButton_8_released(){    //save frame results
    std::lock_guard<std::mutex>lock(matlk);
    if (fitres.empty()) return;

    QString fileName = QFileDialog::getSaveFileName(this,tr("text"), "",tr("Textual (*.txt)"));
    if(fileName.isEmpty()) return;
    std::cout<<"Saving fit results to "<<fileName.toStdString()<<"\n";

    std::ofstream wfile(fileName.toStdString());
    if(wfile.is_open()){
        wfile<<"pos(mm) wdtX(um) wdtY(um) posX(um) posY(um) pow(au) bgn(au) ang(rad) wdtX_err(um) wdtY_err(um) posX_err(um) posY_err(um) pow_err(au) bgn_err(au) ang_err(rad)\n";
        for(int i=0;i!=fitres.size();i++){
            wfile<<i*stepsize<<" "<<fitres[i].wdtX[0]*pixdim<<" "<<fitres[i].wdtY[0]*pixdim<<" "<<fitres[i].posX[0]*pixdim<<" "<<fitres[i].posY[0]*pixdim<<" "<<fitres[i].pow[0]<<" "<<fitres[i].bgn[0]<<" "<<fitres[i].ang[0];
            wfile<<            " "<<fitres[i].wdtX[1]*pixdim<<" "<<fitres[i].wdtY[1]*pixdim<<" "<<fitres[i].posX[1]*pixdim<<" "<<fitres[i].posY[1]*pixdim<<" "<<fitres[i].pow[1]<<" "<<fitres[i].bgn[1]<<" "<<fitres[i].ang[1]<<"\n";
        }
        wfile.close();
    }
}
void MainWindow::on_pushButton_7_released(){    //fit beam: for gaussian beam D4σ == 1/e^2, here we calculate 1/e^2. TODO: implement D4σ instead and get M^2 factor as well
    std::lock_guard<std::mutex>lock(matlk);
    if (fitres.empty()) return;

    gnuplot a;   //TODO: apparently gnuplot replies into cerr instead of cout, or our pipe implementation is broken. investigate!
                 //TODO: bug when calling **persist**, the gnuplot process stays open after program close
    a.POUT<<"set terminal wxt enhanced **persist** size 1500,600\n";
    a.POUT<<"set xlabel \"z / mm\"\n";
    a.POUT<<"set ylabel \"w / um\"\n";
    a.POUT.flush();
    a.POUT<<"wX(x)=w0X*((1+(l*(x-X0)/pi/(w0X**2))**2)**(0.5))\n";
    a.POUT<<"wY(x)=w0Y*((1+(l*(x-Y0)/pi/(w0Y**2))**2)**(0.5))\n";
    a.POUT<<"w0X="<<fitres[0].wdtX[0]<<"\n";
    a.POUT<<"w0Y="<<fitres[0].wdtY[0]<<"\n";
    a.POUT<<"X0="<<-100*stepsize<<"\n";
    a.POUT<<"Y0="<<-100*stepsize<<"\n";
    a.POUT<<"l="<<ui->wavelength->value()<<"\n";
    a.POUT<<"echo = \""<<keyword<<"\"\n";
    a.POUT<<"fit wX(x) \"-\" using 1:2 via w0X, X0\n";
    for(int i=0;i!=fitres.size();i++){
        a.POUT<<i*stepsize<<" "<<fitres[i].wdtX[0]*pixdim<<"\n";
    }
    a.POUT << "e\n";
    a.POUT.flush();
    a.POUT<<"fit wY(x) \"-\" using 1:2 via w0Y, Y0\n";
    for(int i=0;i!=fitres.size();i++){
        a.POUT<<i*stepsize<<" "<<fitres[i].wdtY[0]*pixdim<<"\n";
    }
    a.POUT << "e\n";
    a.POUT<<"print echo\n";
    a.POUT.flush();
    char line[256];
    QString lines;
    while(1){
        a.PERR.getline(line,256);
        //std::cerr<<line<<"\n";
        lines=line;
        if (lines.contains(keyword, Qt::CaseSensitive)) break;
    }

    double w0X[2],w0Y[2],X0[2],Y0[2];
    a.POUT<<"print w0X\n";    a.POUT.flush();
    a.PERR>>w0X[0];
    a.POUT<<"print w0Y\n";    a.POUT.flush();
    a.PERR>>w0Y[0];
    a.POUT<<"print X0\n";    a.POUT.flush();
    a.PERR>>X0[0];
    a.POUT<<"print Y0\n";    a.POUT.flush();
    a.PERR>>Y0[0];
    a.POUT<<"print w0X_err\n";    a.POUT.flush();
    a.PERR>>w0X[1];
    a.POUT<<"print w0Y_err\n";    a.POUT.flush();
    a.PERR>>w0Y[1];
    a.POUT<<"print X0_err\n";    a.POUT.flush();
    a.PERR>>X0[1];
    a.POUT<<"print Y0_err\n";    a.POUT.flush();
    a.PERR>>Y0[1];

    ui->fitresults->setText(QString::fromStdString(util::toString("Fit results:\n\tw0x=",  w0X[0],"±",w0X[1]," um\n\t",
                                                                                  "X0=",  X0[0],"±",X0[1]," mm\n\t",
                                                                                  "w0y=",  w0Y[0],"±",w0Y[1]," um\n\t",
                                                                                  "Y0=",  Y0[0],"±",Y0[1]," mm\n"
                                                                  )));


    a.POUT<<"plot \"-\" using 1:2:3 with errorbars pt 7 lc 6 title \"wX frame fit\", wX(x) w l lc 4 title \"wX total fit\","
          <<     "\"-\" using 1:2:3 with errorbars pt 7 lc 7 title \"wY frame fit\", wY(x) w l lc 10 title \"wY total fit\"\n";
    for(int i=0;i!=fitres.size();i++){
        a.POUT<<i*stepsize<<" "<<fitres[i].wdtX[0]*pixdim<<" "<<fitres[i].wdtX[1]*pixdim<<"\n";
    }
    a.POUT << "e\n";
    for(int i=0;i!=fitres.size();i++){
        a.POUT<<i*stepsize<<" "<<fitres[i].wdtY[0]*pixdim<<" "<<fitres[i].wdtY[1]*pixdim<<"\n";
    }
    a.POUT << "e\n";
    a.POUT.flush();

}
