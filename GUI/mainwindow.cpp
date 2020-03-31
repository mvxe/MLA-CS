#include "GUI/mainwindow.h"
#include "ui_mainwindow.h"
#include "includes.h"
#include <opencv2/highgui/highgui.hpp>  // Video write
#include "UTIL/pipe.h"                  //gnuplot

#include "DEV/ALP/alp.h"        //TODO remove
#include "DEV/TCP_con.h"        //TODO remove

MainWindow::MainWindow(QApplication* qapp, QWidget *parent) : qapp(qapp), QMainWindow(parent), ui(new Ui::MainWindow) {
    mats= new std::vector<cv::Mat>;

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

    tabDev=new tab_devices(ui->tab_dev);
    tabMon=new tab_monitor(ui->tab_rpty_monitor);
    tabCam=new tab_camera(ui->tab_camera);

    tabMon->pmw=this;
}

MainWindow::~MainWindow(){
    delete ui;
    if(!cleanedTabs){
        delete tabDev;
        delete tabMon;
        delete tabCam;
    }
}

void MainWindow::program_exit(){
    timer->stop();
    delete tabDev;
    delete tabMon;
    delete tabCam;
    cleanedTabs=true;
    go.cleanup();
}

void MainWindow::on_tabWidget_currentChanged(int index){
    QString tabName=ui->tabWidget->tabText(index);
    QString lastTabName=ui->tabWidget->tabText(lastIndex);
    lastIndex=index;

    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch

    if (tabName=="UtilCam") utilCam_img->setUserFps(30.,5);
    else if(lastTabName=="UtilCam") utilCam_img->setUserFps(0.);

    if (tabName=="Camera") tabCam->tab_entered();
    else if(lastTabName=="Camera") tabCam->tab_exited();

    tabMon->tab_is_open=(ui->tabWidget->tabText(index)=="Monitor");
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
void MainWindow::on_rpty_rst_btn_released(){
    if(go.pRPTY->connected){
        std::cerr<<"Resetting RPTY.\n";
        go.pRPTY->FIFOreset();
        go.pRPTY->PIDreset();
    }
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
        cv::cvtColor(src, src, cv::COLOR_BGR2GRAY);
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


void MainWindow::on_pushButton_11_clicked(){    //RPTY TEST : TODO REMOVE
//    for(int i=0;i!=4;i++){
//        printf("A2F_RSMax for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::A2F_RSMax,i));
//        printf("A2F_RSCur for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::A2F_RSCur,i));
//        printf("A2F_lostN for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::A2F_lostN,i));
//        printf("F2A_RSMax for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::F2A_RSMax,i));
//        printf("F2A_RSCur for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::F2A_RSCur,i));
//        printf("F2A_lostN for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::F2A_lostN,i));
//    }

//    std::vector<uint32_t> commands;
//    commands.push_back(CQF::W4TRIG_INTR());
//    commands.push_back(CQF::GPIO_MASK(0,0,0xFF));
//    commands.push_back(CQF::GPIO_DIR (0,0,0x00));
//    commands.push_back(CQF::GPIO_VAL (0,0,0x00));
//    commands.push_back(CQF::WAIT(12500000));
//    for(int i=0;i!=8;i++){
//        commands.push_back(CQF::GPIO_MASK(0,0,1<<i));
//        commands.push_back(CQF::GPIO_VAL (0,0,1<<i));
//        commands.push_back(CQF::WAIT(12500000));
//    }
//    for(int i=0;i!=8;i++){
//        commands.push_back(CQF::GPIO_MASK(0,0,1<<i));
//        commands.push_back(CQF::GPIO_VAL (0,0,0));
//        commands.push_back(CQF::WAIT(12500000));
//    }
//    commands.push_back(CQF::GPIO_MASK(0,0,0xFF));
//    commands.push_back(CQF::GPIO_VAL (0,0,0xFF));
//    commands.push_back(CQF::WAIT(12500000));
//    commands.push_back(CQF::GPIO_VAL (0,0,0x00));
//    commands.push_back(CQF::SG_SAMPLE(CQF::O0O1,4000,-8000));
//    go.pRPTY->A2F_write(0,commands.data(),commands.size());

//    commands.clear();
//    commands.push_back(CQF::W4TRIG_INTR());
//    commands.push_back(CQF::ACK(1, 0, CQF::fADC_A__fADC_B, true));
//    commands.push_back(CQF::WAIT(100));
//    commands.push_back(CQF::ACK(1, 0, CQF::fADC_A__fADC_B, false));
//    go.pRPTY->A2F_write(1,commands.data(),commands.size());

//    for(int i=0;i!=4;i++){
//        printf("A2F_RSMax for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::A2F_RSMax,i));
//        printf("A2F_RSCur for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::A2F_RSCur,i));
//        printf("A2F_lostN for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::A2F_lostN,i));
//        printf("F2A_RSMax for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::F2A_RSMax,i));
//        printf("F2A_RSCur for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::F2A_RSCur,i));
//        printf("F2A_lostN for queue %d is %d\n",i,go.pRPTY->getNum(RPTY::F2A_lostN,i));
//    }

//    go.pRPTY->trig(0x3);

//    std::vector<uint32_t> read;
//    read.reserve(100);
//    go.pRPTY->F2A_read(0,read.data(),100);
//    for(int i=0;i!=100;i++) printf("data: N=%d,  MSB=%d, LSB=%d\n",i,AQF::getChMSB(read[i]),AQF::getChLSB(read[i]));



    // small calibration for some other purpose

    uint32_t toread=go.pRPTY->getNum(RPTY::F2A_RSCur,0);
    std::vector<uint32_t> read;
    read.reserve(toread);
    go.pRPTY->F2A_read(0,read.data(),toread);
    read.clear();
std::cerr<<"max "<<go.pRPTY->getNum(RPTY::A2F_RSMax,0)<<"\n";
    int N=100;
    std::vector<long long> read2;
    int avg=4;
    int len=500;
    int extra=10;
    for(int j=0;j!=N;j++){
        std::vector<uint32_t> commands;
                    commands.push_back(CQF::W4TRIG_INTR());
                    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 300, extra*(1<<avg)*len));
                    commands.push_back(CQF::ACK(1, avg, CQF::fADC_A__fADC_B, true));      //TODO remove
                    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 300, extra*(1<<avg)*len));
//                    for(int i=0;i!=11;i++){
//                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 300+i*70, (1<<avg)*700));
//                    }
//                    for(int i=0;i!=70;i++){
//                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 300+i*10, (1<<avg)*70));
//                    }
                    for(int i=0;i!=700;i++){
                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 300+i, (1<<avg)*7));
                    }
                    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 1000, extra*(1<<avg)*len));
                    commands.push_back(CQF::ACK(1, avg, CQF::fADC_A__fADC_B, false));
                    go.pRPTY->A2F_write(0,commands.data(),commands.size());
                                go.pRPTY->trig(1);
                                commands.clear();


                                            do std::this_thread::sleep_for (std::chrono::milliseconds(500));
                                            while(go.pRPTY->getNum(RPTY::A2F_RSCur,0)!=0);

                                            toread=go.pRPTY->getNum(RPTY::F2A_RSCur,0);
                                            std::cerr<<"reading "<<toread<<" num "<<j<<"\n";
                                            read.reserve(toread);
                                            go.pRPTY->F2A_read(0,read.data(),toread);
                                            if(read2.empty()) for(int k=0;k!=toread;k++) read2.push_back(AQF::getChMSB(read[k]));
                                            else for(int k=0;k!=std::min(read2.size(),(unsigned long)toread);k++) read2[k]+=AQF::getChMSB(read[k]);
                                            std::cerr<<"sizes "<<read2.size()<<" "<<toread<<"\n";
    }

    std::ofstream wfile(util::toString("meas4ben/chAcalib2^4Avg700steps.dat"));
    for(int j=0; j!=read2.size(); j++){
        wfile<<j*(1<<avg)*8e-9<<" "<<read2[j]/N<<"\n";
    }
    wfile.close();

}

void MainWindow::on_pushButton_13_released(){
    ALP DMDtest;
    if(!DMDtest.resolve("10.42.0.121",666)) printf("resolved.\n");
    DMDtest.connect(100);

    if(DMDtest.connected){
        ALP::ALP_ID DeviceIdPtr;
        int ret;
        DMDtest.AlpDevAlloc(ALP::ALP_DEFAULT,ALP::ALP_DEFAULT, &DeviceIdPtr);
        DMDtest.AlpDevInquire(DeviceIdPtr, ALP::ALP_DEVICE_NUMBER, &ret);
        printf("Device number: %d\n",ret);
        DMDtest.AlpDevInquire(DeviceIdPtr, ALP::ALP_VERSION, &ret);
        printf("Device version: %d\n",ret);
        DMDtest.AlpDevInquire(DeviceIdPtr, ALP::ALP_DEV_DMDTYPE, &ret);
        printf("DMD type: %d\n",ret);
        DMDtest.AlpDevInquire(DeviceIdPtr, ALP::ALP_AVAIL_MEMORY, &ret);
        printf("Available memory (Num of images): %d\n",ret);

        ALP::ALP_ID SeqID;
        if (ALP::ALP_OK != DMDtest.AlpSeqAlloc( DeviceIdPtr, 1, 2, &SeqID )) return;
        unsigned char *pImageData=nullptr;
        long nSizeX=1920, nSizeY=1200;
        const long nPictureTime = 200000;	// 200 ms, i.e. 5 Hz frame rate
        pImageData = new unsigned char[2*nSizeX*nSizeY];
        if (nullptr == pImageData) return;
        for(long i=0;i!=nSizeX*nSizeY;i++) pImageData[i]=0x80;                  // white
        for(long i=0;i!=nSizeX*nSizeY;i++) pImageData[i+nSizeX*nSizeY]=0x00;    // black
        long nReturn = DMDtest.AlpSeqPut( DeviceIdPtr, SeqID, 0, 2, pImageData, 2*nSizeX*nSizeY);
        delete[] pImageData;
        if (ALP::ALP_OK != nReturn) {printf("error, nreturn=%d\n",nReturn); return;}
        if (ALP::ALP_OK != DMDtest.AlpSeqTiming( DeviceIdPtr, SeqID, ALP::ALP_DEFAULT, nPictureTime,ALP::ALP_DEFAULT, ALP::ALP_DEFAULT, ALP::ALP_DEFAULT ) ) {printf("error 2\n"); return;}
        if (ALP::ALP_OK != DMDtest.AlpProjStartCont( DeviceIdPtr, SeqID )) return;

        std::this_thread::sleep_for (std::chrono::seconds(3));

        DMDtest.AlpDevHalt(DeviceIdPtr);
        DMDtest.AlpDevFree(DeviceIdPtr);
        DMDtest.disconnect();
        printf("success\n");
    }
    else printf("failed to connect\n");
}
