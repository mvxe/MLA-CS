#include "GUI/mainwindow.h"
#include "includes.h"
globals go;

globals::globals(){}
globals::~globals(){}

QApplicationQN::QApplicationQN(int& argc, char** argv):QApplication(argc, argv){}
bool QApplicationQN::notify(QObject* receiver, QEvent* event) {             // when things throw exceptions in Qt threads, terminate is not called, so we redefine this to show the exeptions
    bool done=true;
    try{
        done=QApplication::notify(receiver, event);
    }catch(const std::exception& e){
        std::cerr << "Caught exception (in a Qt thread): \n" << e.what() << "\n";
        this->quit();
    }
    return done;
}

void myterminate() {                                                        // if an exception is thrown which calls terminate, output the exception here and exit peac
    go.markErrord();
    std::cerr<<">>> Terminate function called!\n";
    std::exception_ptr exc = std::current_exception();
    try{
        std::rethrow_exception(exc);
    }
    catch(const std::exception& e) {
        std::cerr << "Caught exception: \n" << e.what() << "\n";
    }
    std::thread th(&globals::quit, &go);
    th.detach();
    //while(1) std::this_thread::sleep_for (std::chrono::seconds(99999));
}

void globals::startup(int argc, char *argv[]){
    qapp = new QApplicationQN(argc, argv);
    if (started) return; started=true;
    std::set_terminate(myterminate);
    //cinthr=new std::thread(&cinlisten::run, new cinlisten());            //so that I can kill it in the terminal if qt freezes, which is often over ssh -x

    // registering nanostructuring setup devices, threads
    std::string CTRL_TYPE="ctrl_RPTY";       // default to RPTY for backward compatibility
    conf["CTRL_TYPE"]=CTRL_TYPE;
    conf.load();
    if(CTRL_TYPE=="ctrl_RPTY"){
        pCTRL=newThread<RPTY>()->obj;
        conf["RPTY"]=pCTRL->conf;                           // keep RPTY name for backward compatibility, TODO change in future if more devices are implemented
    }else{
        conf["CTRL_TYPE"].comments().push_back("Implemented contollers: ctrl_RPTY.");
        conf["CTRL_TYPE"].save();
        throw std::runtime_error(util::toString("In globals::startup, uncrecognized/nonexistant contoller for nanostructuring: ",CTRL_TYPE,". Saved possible options to conf."));
    }
    GUIdevList.push_back(pCTRL);
    pCTRL->registerDevice("X", CTRL::dt_motion);
    pCTRL->registerDevice("Y", CTRL::dt_motion);
    pCTRL->registerDevice("Z", CTRL::dt_motion);
    pCTRL->registerDevice("XTilt", CTRL::dt_motion);
    pCTRL->registerDevice("YTilt", CTRL::dt_motion);
    pCTRL->registerDevice("trigCam", CTRL::dt_gpio);
    pCTRL->registerDevice("wrLaser", CTRL::dt_gpio);
    pCTRL->registerDevice("ilumLED", CTRL::dt_gpio);
    pCTRL->registerDevice("timer", CTRL::dt_timer);
    // done registering devices

    // all cams go here
    // for now define cameras in /DEV/GCAM/_config.h
    // nanostructuring camera is named iuScope
    pGCAM=newThread<GCAM>()->obj;
    GUIdevList.push_back(pGCAM);
    //std::this_thread::sleep_for (std::chrono::milliseconds(100));
    conf["GCAM"]=pGCAM->conf;

    conf.load();
    pGCAM->update();

    MainWindow w(qapp);
    w.setWindowTitle("MLA-CS");
    w.show();
    try {
        qapp->exec();
    } catch (...) {
        cleanup();
        return;
    }
}
void globals::killThread(base_othr*& thro){
    std::lock_guard<std::mutex>lock(tdmx);
    bool found=false;
    for (std::list<base_othr*>::iterator it1=threads.begin(); it1!=threads.end(); ++it1){
        if(*it1==thro) {
            threads.erase(it1);
            found = true; break;
        }
    }
    if (found) delete thro;
    else std::cout<<"Something went wrong in globals::killThread.\n";
    thro=nullptr;
}
void globals::cleanup(){
    if (!go_mx.try_lock()) return;
    std::cout<<"Saving conf...\n";
    if(conf.changed()) conf.save(); // the threads should save on exit themselves

    std::cout<<"Sending end signals to all threads...\n";

    while(!threads.empty())
        killThread(threads.front());        //this destroys them(safely calling their destructors)

    std::cout<<"All threads exited successfully!\n";
    go_mx.unlock();
}
void globals::quit(){
    qapp->quit();
}
void globals::markErrord(){
    for (std::list<base_othr*>::iterator it1=threads.begin(); it1!=threads.end(); ++it1){
        if((*it1)->thr->get_id()==std::this_thread::get_id()) {
            (*it1)->errord=true;
            return;
        }
    } std::cerr<<"Cannot find errord thread id (see markErrord)\n";
}
