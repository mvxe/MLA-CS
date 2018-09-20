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
#include <list>
#include <iostream>
#include <UTIL/containers.h>
#include <UTIL/utility.h>

class XPS;
class MAKO;
class RPTY;
class QApplication;

class var_save{
public:
    var_save(std::string filename);
    ~var_save();
    _savelst save;
private:
    std::string filename;
};

class protooth{              //abstract class for all classes to be in threads
    friend class base_othr;
public:
    virtual ~protooth(){*done=true;}                     //if its virtual, destroying its pointer will call the derived class destructor
    std::atomic<bool> end{false};
    std::atomic<bool>* done;
private:
    virtual void run()=0;
};

class base_othr{
public:
    virtual ~base_othr(){}
    std::atomic<bool> done{false};      //this flag indicates whether the thread has closed, it is then safe to destroy this object. It is automatically set by the destructor od any protooth derived class.
protected:
    std::thread* thr;
};
template <typename T>
class othr: public base_othr{
public:
    othr();
    ~othr();
    T* obj;
};
template <typename T>
othr<T>::othr(){
    obj=new T();
    obj->done=&done;
    thr=new std::thread(&protooth::run, obj);
}
template <typename T>
othr<T>::~othr(){
    obj->end=true;
    thr->join();
    delete obj;
    delete thr;
}

class globals{  //global objects with thread safe public functions
public:
    globals();
    ~globals();

    var_save config{"general.conf"};                    //this is initialized before everything else and before the globals() is called
    var_save cams_config{"cameras.conf"};
    var_save gui_config{"gui.conf"};
    var_save pos_config{"positioners.conf"};

    XPS* pXPS;      //you can access stages through this
    MAKO* pMAKO;    //you can access cameras and frame queues through this, see MAKO/_config.h for members
    RPTY* pRPTY;    //you can access red pitaya functions through this

    void startup(int argc, char *argv[]);                         //subsequent calls of this are ignored
    template <typename T> othr<T>* newThread();                   //with this you can create a new thread calling any object derived from protooth/procedure, example:  XPS* pXPS=newThread<XPS>()->obj; or othr<proc> name=newThread<proc>();
    void killThread(base_othr *&thro);                            //this kills the thread, ie if the thread is still running it sends an end flag and waits till its done, if its done its simply deallocated and removed from the list
                                                                  //you may check if the thread is done by looking at the base_othr.done or directly at the objects done variable, however after killThread is called the object WILL be deallocated
    void cleanup();
    void quit();                                                  //sends quit signal to the app, equivalent to clicking X on the window (not sure if thread safe, but its for exit on error anyway, to trigger apis cleanup before exit)
            /* use go.quit() in any thread if you need to quit the program*/
private:
    std::mutex go_mx;
    bool started{false};
    std::list<base_othr*> threads;
    std::thread* cinthr;
    QApplication* qapp;
};

/*########## TEMPLATE FUNCTIONS ##########*/

template <typename T>
othr<T>* globals::newThread(){
    if (!std::is_base_of<protooth, T>::value) {
        std::cerr << "ERROR: You called newThread on a class not derived from protooth/procedure.\n";
        quit(); return nullptr;
    }
    threads.emplace_front(new othr<T>());
    return dynamic_cast<othr<T>*>(threads.front());
}

/*####################*/

extern globals go;

class cinlisten{
public:
    void run(){
        std::string txt;
        for(;;){
            std::cin>>txt;
            if (txt.find("exit")!=std::string::npos) break;
        }
        std::thread ttry(&globals::quit, &go);                          //we tell QT to exit nicely
        std::this_thread::sleep_for (std::chrono::seconds(2));          //after 2 seconds if QT hasnt exited, stop threads and kill QT
        go.cleanup();                                                   //this does nothing if threads started to getting killed
        exit(0);
    }
};

#endif // GLOBALS_H
