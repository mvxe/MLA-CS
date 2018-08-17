#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    px_online = new QPixmap(":/emblem-ok.svg");
    px_offline = new QPixmap(":/emblem-nowrite.svg");

    ui->setupUi(this);
    sync_settings();
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(GUI_update()));
    timer->start(100);

    menu = new QMenu(this);
    menu->setToolTipsVisible(true);
    ui->cam1_select->setMenu(menu);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(cam1_select_show()));
}

MainWindow::~MainWindow(){
    std::cout<<"Sending end signals to all threads...\n";
    std::vector<mxvar<bool>*> tokill;
    timer->stop();


    tokill.push_back(&sw.XPS_end);     //this one closes last
    tokill.push_back(&sw.MAKO_end);
    //tokill.push_back(&RPTY_end);

    while(!tokill.empty()){           //the threads are closed one by one, although it takes longer, we can now define the order above
        tokill.back()->set(true);     //signal the other threads to exit
        while (tokill.back()->get())std::this_thread::sleep_for (std::chrono::milliseconds(100));    //wait for the thread to exit
        tokill.pop_back();
    }

    std::cout<<"All threads exited successfully!\n";
    delete ui;
}


void MainWindow::on_tabWidget_currentChanged(int index){
    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch
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
