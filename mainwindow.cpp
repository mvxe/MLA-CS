#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    px_online = new QPixmap(":/emblem-ok.svg");
    px_offline = new QPixmap(":/emblem-nowrite.svg");

    ui->setupUi(this);
    sync_settings();
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(GUI_update()));
    timer->start(100);
}

MainWindow::~MainWindow(){
    std::cout<<"Sending end signals to all threads...\n";
    std::vector<mxvar<bool>*> tokill;

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
