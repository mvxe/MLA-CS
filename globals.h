/* Here be pointers to global objects.
 * Generally, public functions of these should be thread safe.
 * This also handles variables that are saved to file
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <mutex>
#include <thread>
#include <string>
#include <atomic>
#include <vector>
#include <list>
#include <iostream>
#include <UTIL/containers.h>
#include <UTIL/utility.h>
#include <UTIL/threadpool.h>
#include <UTIL/.rtoml/rtoml.hpp>

class GCAM;
class CTRL;
class QApplication;
class MainWindow;

#include <exception>
#include <QApplication>
class QApplicationQN:public QApplication{   //we redefine QApplication to add exception handling for things thrown withing Qt threads
public:
    QApplicationQN(int& argc, char** argv);
    bool notify(QObject* receiver, QEvent* event);
};

class globals;
class base_othr{
    friend class globals;
public:
    base_othr(std::atomic<bool>& end, std::atomic<bool>& done): end(end), done(done){}
    virtual ~base_othr(){}
    std::atomic<bool>& end;
    std::atomic<bool>& done;
protected:
    std::thread* thr;
    std::atomic<bool> errord{false};
};
template <typename T>
class othr: public base_othr{
public:
    template <typename... Args> othr(Args... args);
    ~othr();
    T* obj;
};
template <typename T> template <typename... Args>
othr<T>::othr(Args... args): obj(new T(args...)), base_othr(obj->end,obj->done){
    thr=new std::thread(&protooth::run, obj);
}
template <typename T>
othr<T>::~othr(){
    if (!errord){
        obj->end=true;
        thr->join();
        //delete obj;       // TODO fix : this is at the end of program so it doesn't matter, but ideally this should be deleted however that prevends configuration saving to work as the referenced objects are deleted
        delete thr;
    }
}

class globals{  //global objects with thread safe public functions
    friend void cleanTerminate();
public:
    globals();
    ~globals();

    GCAM* pGCAM;    //you can access cameras and frame queues through this, see GCAM/_config.h for members
    CTRL* pCTRL;    //you can access the controller's functions through this
    std::vector<protooth*> GUIdevList;      // contains device connection GUI

    rtoml::vsr conf{"devices.toml"};
    std::vector<rtoml::vsr*> confList;      // contains pointers to all config files (conf, tab confs...)
    threadPool OCL_threadpool{16};   //apparently opencl does not do well with threads: depending on the driver it fails after usage on a number (~200) of different threads. Using threadpool apparently fixes this, hence OCL_threadpool

    void startup(int argc, char *argv[]);                                           //subsequent calls of this are ignored
    template <typename T, typename... Args> othr<T>* newThread(Args... args);       //with this you can create a new thread calling any object derived from protooth/procedure, example:  XPS* pXPS=newThread<XPS>()->obj; or othr<proc> name=newThread<proc>();
    void killThread(base_othr *&thro);                                              //this kills the thread, ie if the thread is still running it sends an end flag and waits till its done, if its done its simply deallocated and removed from the list
                                                                                    //you may check if the thread is done by looking at the base_othr.done or directly at the objects done variable, however after killThread is called the object WILL be deallocated                                                           
    void cleanup();
    void quit();                                                                    //sends quit signal to the app, equivalent to clicking X on the window (not sure if thread safe, but its for exit on error anyway, to trigger apis cleanup before exit)
            /* use go.quit() in any thread if you need to quit the program*/

    void markErrord();          //if the thread throws an exception
private:
    std::mutex go_mx;
    bool started{false};
    std::list<base_othr*> threads;
    std::mutex tdmx;
    std::thread* cinthr;
    QApplicationQN* qapp;
    MainWindow* mw;
};

/*########## TEMPLATE FUNCTIONS ##########*/

template <typename T, typename... Args>
othr<T>* globals::newThread(Args... args){
    std::lock_guard<std::mutex>lock(tdmx);
    if (!std::is_base_of<protooth, T>::value) {
        std::cerr << "ERROR: You called newThread on a class not derived from protooth/procedure.\n";
        quit(); return nullptr;
    }
    threads.emplace_front(new othr<T>(args...));
    return dynamic_cast<othr<T>*>(threads.front());
}

/*####################*/

extern globals go;

//class cinlisten{
//public:
//    void run(){
//        std::string txt;
//        for(;;){
//            std::cin>>txt;
//            if (txt.find("exit")!=std::string::npos) break;
//        }
//        std::thread ttry(&globals::quit, &go);                          //we tell QT to exit nicely
//        std::this_thread::sleep_for (std::chrono::seconds(2));          //after 2 seconds if QT hasnt exited, stop threads and kill QT
//        go.cleanup();                                                   //this does nothing if threads started to getting killed
//        exit(0);
//    }
//};

#endif // GLOBALS_H
