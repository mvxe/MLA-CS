/* This is the procedure template class. Use it as a base for new procedures.
 */
#include <atomic>
#include "globals.h"

#ifndef PROCEDURE_H
#define PROCEDURE_H


class procedure: public protooth{
public:
    procedure();
    virtual ~procedure();
    std::atomic<bool> end{false};           // this can be used by the creator thread or go.cleanup() to shut down the thread

protected:
    virtual bool init();                    /* you may override this function, it gets called on thread start (NOTE: this and cleanup are executed in the new thread, in contrast to constructor and destructor actually executed on the thread calling newThread and killThread)
                                             * return true on failed init, it will stop the thread without calling work()
                                             */
    virtual void cleanup();                 // you may override this function, it gets called on thread end
    virtual bool work()=0;                  /* you must override this function, it gets called from run() when the new thread is started,  upon returning true it exits the thread
                                             * if it returns false, it gets called again after end variable is checked
                                             * using work() cycle by cycle like this allows the main thread to kill the thread if needed, otherwise a long procedure cannot be killed and may freeze the program
                                             */
private:
    virtual void run() final;                             // this function gets called by the new thread
};

#endif // PROCEDURE_H
