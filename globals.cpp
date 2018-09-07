#include "globals.h"
globals go;


globals::globals() : started(false){}
globals::~globals(){}
void globals::startup(){
    if (started) return; started=true;

    cURLpp::initialize(CURL_GLOBAL_ALL);     //we init the curl lib needed for FTP

    pXPS=new XPS();
    XPS_thread = std::thread(&XPS::run, pXPS);

    pMAKO=new MAKO();
    MAKO_thread = std::thread(&MAKO::run, pMAKO);
}
void globals::cleanup(){
    if (!go_mx.try_lock()) return;
    std::cout<<"Sending end signals to all threads...\n";

    sw.MAKO_end.set(true);
    MAKO_thread.join();
    delete pMAKO;

    sw.XPS_end.set(true);
    XPS_thread.join();
    delete pXPS;

    cURLpp::terminate();                    //we terminate curl
    std::cout<<"All threads exited successfully!\n";
    go_mx.unlock();
}
