#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]){
    QApplication* qapp = new QApplication(argc, argv);
    MainWindow w(qapp);
    w.show();

    go.startup();
    qapp->exec();
}
