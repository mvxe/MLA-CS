#include "GUI/mainwindow.h"
#include <QApplication>
#include "includes.h"
globals go;

globals::globals(){}
globals::~globals(){}
void globals::startup(int argc, char *argv[]){
    if (started) return; started=true;
    cURLpp::initialize(CURL_GLOBAL_ALL);     //we init the curl lib needed for FTP

    pXPS =newThread<XPS> ()->obj;
    pMAKO=newThread<MAKO>()->obj;
    pRPTY=newThread<RPTY>()->obj;

    qapp = new QApplication(argc, argv);
    MainWindow w(qapp);
    w.show();
    qapp->exec();
}
template <typename T>
othr<T>* globals::newThread(){
    if (!std::is_base_of<protooth, T>::value) {
        std::cerr << "ERROR: You called newThread on a class not derived from protooth/procedure.\n";
        quit(); return nullptr;
    }
    threads.emplace_front(new othr<T>());
    return dynamic_cast<othr<T>*>(threads.front());
}
void globals::killThread(base_othr*& thro){
    delete thro;
    for (std::list<base_othr*>::iterator it1=threads.begin(); it1!=threads.end(); ++it1)
        if(*it1==thro) {
            threads.erase(it1);
            break;
        }
    thro=nullptr;
}
void globals::cleanup(){
    if (!go_mx.try_lock()) return;
    std::cout<<"Sending end signals to all threads...\n";

    while(!threads.empty())
        killThread(threads.front());        //this destroys them(safely calling their destructors)

    cURLpp::terminate();                    //we terminate curl
    std::cout<<"All threads exited successfully!\n";
    go_mx.unlock();
}
void globals::quit(){
    qapp->quit();
}

/* config_save */

#define _CODE0_ " [val] "
#define _NCHC0_ 7                  //num of characters in code
#define _CODE1_ " [var] "
#define _NCHC1_ 7                   //the config is saved into a plain text file in format VAR1NAME_CODE0_VAR1VALUE_CODE1_VAR2NAME_CODE0_... and so on without newlines
var_save::var_save(std::string filename): filename(filename){   //at program start we read from file
    std::ifstream ifile;
    std::stringstream buffer;
    ifile.open (filename.c_str());
    buffer << ifile.rdbuf();
    ifile.close();
    int iter=buffer.str().find(_CODE1_,0), iter2, iter3;
    if (iter!=std::string::npos)
        for (;;iter=iter3){
            iter2 = buffer.str().find(_CODE0_,iter+_NCHC0_);
            if (iter2 == std::string::npos) break;
            iter3 = buffer.str().find(_CODE1_,iter2+_NCHC1_);
            if (iter3 == std::string::npos) break;
            save.push_back({buffer.str().substr(iter+_NCHC0_,iter2-iter-_NCHC0_),buffer.str().substr(iter2+_NCHC1_,iter3-iter2-_NCHC1_)});
        }
}
var_save::~var_save(){  //at program end we write to file
    std::ofstream ofile;
    ofile.open (filename.c_str(),std::ofstream::trunc);
    ofile << _CODE1_;
    for (int i=0;i!=save.size();i++)
        ofile << save[i].strname << _CODE0_ << save[i].strval << _CODE1_;
    ofile.close();
}
