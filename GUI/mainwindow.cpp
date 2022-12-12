#include "GUI/mainwindow.h"
#include "DEV/GCAM/gcam.h"
#include "ui_mainwindow.h"
#include <opencv2/highgui/highgui.hpp>  // Video write

MainWindow::MainWindow(QApplication* qapp, QWidget *parent) : qapp(qapp), QMainWindow(parent), ui(new Ui::MainWindow) {
    connect(qapp,SIGNAL(aboutToQuit()),this,SLOT(program_exit()));
    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q),this,SLOT(program_exit()));

    ui->setupUi(this);

    tabDev=new tab_devices(ui->tab_dev);
    for(auto& dev: go.GUIdevList){
        for(auto& widget: dev->connectionGUI)
            tabDev->addWidget(widget);
    }

    tabCam=new tab_camera(ui->tab_camera);

    setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
}

MainWindow::~MainWindow(){
    delete ui;
    if(!cleanedTabs){
        delete tabDev;
        delete tabCam;
    }
}

void MainWindow::program_exit(){
    delete tabDev;
    delete tabCam;
    cleanedTabs=true;
    go.cleanup();
}

void MainWindow::on_tabWidget_currentChanged(int index){
    QString tabName=ui->tabWidget->tabText(index);
    QString lastTabName=ui->tabWidget->tabText(lastIndex);
    lastIndex=index;

    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch

    if (tabName=="Nanostructuring") tabCam->tab_entered();
    else if(lastTabName=="Nanostructuring") tabCam->tab_exited();
}
