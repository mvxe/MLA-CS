#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QApplication* qapp, QWidget *parent) : qapp(qapp), QMainWindow(parent), ui(new Ui::MainWindow) {
    connect(qapp,SIGNAL(aboutToQuit()),this,SLOT(program_exit()));
    px_online = new QPixmap(":/emblem-ok.svg");
    px_offline = new QPixmap(":/emblem-nowrite.svg");

    ui->setupUi(this);
    sync_settings();
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(GUI_update()));
    timer->start(10);

    menu = new QMenu(this);
    menu->setToolTipsVisible(true);
    ui->cam1_select->setMenu(menu);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(cam1_select_show()));
}

MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::program_exit(){
    std::cout<<"Sending end signals to all threads...\n";

    timer->stop();
    sw.XPS_end.set(true);
    XPS_thread.join();
    sw.MAKO_end.set(true);
    MAKO_thread.join();

    std::cout<<"All threads exited successfully!\n";
}

void MainWindow::on_tabWidget_currentChanged(int index){
    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch
    if (ui->tabWidget->tabText(index)=="Camera") sw.iuScope_img->setUserFps(30.,50);
    else sw.iuScope_img->setUserFps(0.);
}


void MainWindow::cam1_select_show(){   //on_cam1_select pressed, should update usb device list
    updateCamMenu();
}
void MainWindow::on_cam1_select_triggered(QAction *arg1){   //on_cam1_select action selected
    sw.iuScopeID.set(arg1->text().toStdString());
    ui->cam1_select->setText("camera ID: "+arg1->text());
    sw.MAKO_reco.set(true);
}

void MainWindow::updateCamMenu(){
    menu->clear();
    while (!actptrs.empty()){   //free old menus
        delete actptrs.back();
        actptrs.pop_back();
    }
    for (int i=0;i!=sw.MAKO_cam_desc.get()->size();i++){
        QAction *actx = new QAction(this);
        actx->setText(QString::fromStdString(sw.MAKO_cam_desc.get()->at(i).ID));
        actx->setToolTip(QString::fromStdString(sw.MAKO_cam_desc.get()->at(i).description));
        menu->addAction(actx);
        actptrs.push_back(actx);
    }
}
