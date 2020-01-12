#ifndef PIPE_TEMPLATE_H
#define PIPE_TEMPLATE_H
#include "pipe.h"

template<typename... Args> _pipe::_pipe(const char *path, const char *arg, Args... args){
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
      execl(path, arg, args...);
      exit(0);
    }
    fbOUT=new __gnu_cxx::stdio_filebuf<char>(pipeo[1], std::ios::out);
    _POUT=new std::ostream(fbOUT);
     fbIN=new __gnu_cxx::stdio_filebuf<char>(pipei[0],  std::ios::in);
     _PIN=new std::istream( fbIN);
    fbERR=new __gnu_cxx::stdio_filebuf<char>(pipee[0],  std::ios::in);
    _PERR=new std::istream(fbERR);
}

#endif // PIPE_TEMPLATE_H
