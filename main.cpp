#include "mainwindow.h"
#include <QApplication>

std::thread XPS_thread;
std::thread MAKO_thread;

int main(int argc, char *argv[])
{
    QApplication* qapp = new QApplication(argc, argv);
    MainWindow w(qapp);
    w.show();

    go.startup();

    qapp->exec();
}
