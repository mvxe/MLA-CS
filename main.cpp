#include "mainwindow.h"
#include <QApplication>
#include "communication_methods.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

//    std::string ret;
//    ComM_XPS xyz_ctrl;
//    std::cout << "connecting...\n";
//    try { xyz_ctrl.connect("192.168.0.254","5001"); }
//    catch (int &a) {std::cout << "error:" << a << "\n";}
//    std::cout << "done.\n";


//    try{ xyz_ctrl.write("GroupInitialize (GROUP1)"); }
//    catch (boost::system::system_error err) { std::cout << err.what();}
//    xyz_ctrl.read(ret);    std::cout << ret;
//    std::cout << "\n";

//    xyz_ctrl.write("GroupHomeSearchAndRelativeMove (GROUP1,0)");
//    xyz_ctrl.read(ret);    std::cout << ret;
//    std::cout << "\n";

//    xyz_ctrl.write("GroupKill (GROUP1)");
//    xyz_ctrl.read(ret);    std::cout << ret;
//    std::cout << "\n";

//    xyz_ctrl.rw("FirmwareVersionGet (char *)",ret);
//    std::cout << ret;

//    std::cout << "\n";

    return a.exec();
}
