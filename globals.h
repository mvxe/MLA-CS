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
#include <stack>
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
    friend class othr;
public:
    virtual ~protooth();     //if its virtual, destroying its pointer will call the derived class destructor
    std::atomic<bool> end{false};
private:
    virtual void run();
};

class othr{
public:
    othr(protooth* newobj);
    ~othr();
    protooth* obj;
protected:
    std::thread* thr;
};

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
    template <typename T> T* newThread(T* procedure);             //with this you can create a new thread calling any object derived from protooth/procedure, example:  procedure* ptr=newThread(new procedure());
    void cleanup();
    void quit();                                                  //sends quit signal to the app, equivalent to clicking X on the window (not sure if thread safe, but its for exit on error anyway, to trigger apis cleanup before exit)
            /* use go.quit() in any thread if you need to quit the program*/
private:
    std::mutex go_mx;
    bool started{false};
    std::stack<othr> threads;
    QApplication* qapp;
};

extern globals go;

#endif // GLOBALS_H
