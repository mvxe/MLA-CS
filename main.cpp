#include "mainwindow.h"
#include <QApplication>
#include "XPS/xps.h"
#include "MAKO/mako.h"


std::thread XPS_thread;
std::thread MAKO_thread;

int main(int argc, char *argv[])
{
    QApplication* qapp = new QApplication(argc, argv);
    MainWindow w(qapp);
    w.show();

    XPS XPS;
    sw.XPSa=&XPS;
    XPS_thread = std::thread(&XPS::run, &XPS);
    MAKO MAKOa;
    MAKO_thread = std::thread(&MAKO::run, &MAKOa);

 //   ComM_XPS xyz_ctrl;
//    std::cout << iCC_GUI.GUI_change();

//    std::string ret;

//    std::cout << "connecting...\n";
//    try { xyz_ctrl.connect("192.168.0.254",5001); }
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

    qapp->exec();
}
