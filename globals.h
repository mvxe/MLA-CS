/* Here be pointers to global objects.
 * Generally, public functions of these should be thread safe.
 * This also handles variables that are saved to file
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <mutex>
#include <thread>
#include <string>
#include <QApplication>
#include <UTIL/containers.h>
class XPS;
class MAKO;
class FQ;
class camobj;

class var_save{
public:
    var_save(std::string filename);
    ~var_save();
    _savelst save;
private:
    std::string filename;
};

class globals{  //global objects with thread safe public functions
public:
    globals();
    ~globals();

    var_save config = var_save("mla_cs2.conf");                    //this is initialized before everything else and before the globals() is called  //TODO change filename
//  var_save somefile2 = var_save("somefile2.conf");               //more save files can be added here, access by config.save

    XPS* pXPS;
    MAKO* pMAKO;

    FQ* iuScope_img;                                              //iuScope -> GUI
    camobj* iuScope_st;                                           //for accessing camera features and settings (the thread safe functions are public, but check if its live first)

    void startup(int argc, char *argv[]);                         //subsequent calls of this are ignored
    void cleanup();
    void quit();                                                  //sends quit signal to the app, equivalent to clicking X on the window (not sure if thread safe, but its for exit on error anyway, to trigger apis cleanup before exit)
            /* use go.quit() in any thread if you need to quit the program*/
private:
    std::mutex go_mx;
    bool started;
    std::thread XPS_thread;
    std::thread MAKO_thread;
    QApplication* qapp;
};

extern globals go;

#endif // GLOBALS_H
