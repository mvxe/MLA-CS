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
    delete ui;
}

