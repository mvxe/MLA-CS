#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]){
    go.startup();

    QApplication* qapp = new QApplication(argc, argv);
    MainWindow w(qapp);
    w.show();
    qapp->exec();
}
