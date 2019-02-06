#include "pipe.h"
#include <unistd.h>
#include <signal.h>

_pipe::_pipe(std::string process){
    int pipeo[2];
    int pipei[2];
    int pipee[2];
    ::pipe(pipeo);      //the [0] is the reading end, [1] is the writing end    //we use this one for main program -> child
    ::pipe(pipei);                                                              //we use this one for child program -> main
    ::pipe(pipee);
    _PID=fork();        //at this point the program forks into two processes continuing execution at the same point, the parent process has the PID containing the actual PID of the child process, while the child process gets PID=0
    if (_PID==0){       //this is true for the child process
      dup2(pipeo[0], STDIN_FILENO);
      dup2(pipei[1], STDOUT_FILENO);
      dup2(pipee[1], STDERR_FILENO);
      execl(process.c_str(), (char*)NULL);
      exit(0);
    }
    fbOUT=new __gnu_cxx::stdio_filebuf<char>(pipeo[1], std::ios::out);
    _POUT=new std::ostream(fbOUT);
     fbIN=new __gnu_cxx::stdio_filebuf<char>(pipei[0],  std::ios::in);
     _PIN=new std::istream( fbIN);
    fbERR=new __gnu_cxx::stdio_filebuf<char>(pipee[0],  std::ios::in);
    _PERR=new std::istream(fbERR);
}

_pipe::~_pipe(){
    fbOUT->close();
     fbIN->close();
    fbERR->close();
    delete fbOUT;
    delete _POUT;
    delete  fbIN;
    delete  _PIN;
    delete fbERR;
    delete _PERR;
//    pid_t killer=fork();      //TODO weird behaviour with beamshapef fit, fix this
//    if(killer==0){          //We create a new process that sleeps for 10 seconds and then kills it. This is in case the user doesn't tell the process to quit or it glitches.
//        ::sleep(10);
//        ::kill(_PID, SIGKILL);
//        exit(0);
//    }
}
