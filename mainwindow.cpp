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
    XPS_end.set(true);     //signal the other threads to exit
    MAKO_end.set(true);
    RPTY_end.set(true);

    while (XPS_end.get() || MAKO_end.get()) std::this_thread::sleep_for (std::chrono::milliseconds(100));       //wait for others to exit, TODO add other threads here
    delete ui;
}


void MainWindow::on_tabWidget_currentChanged(int index){
    ui->centralWidget->setFocus();
}
