/* Here be pointers to global objects.
 * Generally, public functions of these should be thread safe.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "MAKO/frame_queues.h"
#include "XPS/xps.h"
#include "MAKO/mako.h"
#include <mutex>
#include <thread>

class globals{              //global objects with thread safe public functions
public:
    globals();
    ~globals();

    XPS* pXPS;
    MAKO* pMAKO;

    FQ* iuScope_img;                                              //iuScope -> GUI
    camobj* iuScope_st;                                           //for accessing camera features and settings (the thread safe functions are public, but check if its live first)

    void startup();                                               //subsequent calls of this are ignored
    void cleanup();
private:
    std::mutex go_mx;
    bool started;
    std::thread XPS_thread;
    std::thread MAKO_thread;
};

extern globals go;

#endif // GLOBALS_H
