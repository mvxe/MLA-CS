/* This is the procedure template class. Use it as a base for new procedures.
 */
#include <atomic>
#include "globals.h"

#ifndef PROCEDURE_H
#define PROCEDURE_H


class procedure: public protooth{           //see PROCEDURES/procedure.h run() for how these functions are called
public:
    procedure();
    virtual ~procedure();                   // note: this is called when the thread object is destroyed by killThread(), and not when run() returns
    std::atomic<bool> end{false};           // this can be used by the creator thread or go.cleanup() to shut down the thread

protected:
    virtual bool startup();                 /* you may override this function, it gets called on thread start (NOTE: this and cleanup are executed in the new thread, in contrast to constructor and destructor actually executed on the thread calling newThread and killThread)
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


/*############### ALTERNATIVELY, A SECOND TYPE OF PROCEDURE BELOW ###############
 * this one is simpler and has only a base (run()), but you should
 * check end variable to check and handle possible end
 * requests from outside
 */

class sproc: public protooth{               //see PROCEDURES/procedure.h run() for how these functions are called
public:
    virtual ~sproc(){}                      // note: this is called when the thread object is destroyed by killThread(), and not when run() returns
    std::atomic<bool> end{false};           // this can be used by the creator thread or go.cleanup() to shut down the thread, check it periodically from run() and if true do a cleanup and call return asap
protected:
    virtual void run()=0;                   // this function gets called by the new thread, it needs to be redefined by the user
};

#endif // PROCEDURE_H
