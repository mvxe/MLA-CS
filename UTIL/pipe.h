#ifndef PIPE_H
#define PIPE_H

/* this starts an external program and pipes data to and from it
 * it can be used for example with gnuplot to both plot data and
 * fit data and retrieve results, or perhaps some other interactive
 * terminal program.
 *
 * You can use it directly, or as a template for specific programs.
 */

#include <string>
#include <fstream>
#include <ext/stdio_filebuf.h>
#include <iostream>

class _pipe{    //do not use this, use the class 'pipe'
public:
    _pipe(std::string process);
    ~_pipe();
    std::ostream* _POUT;      //for sending data to the program
    std::istream* _PIN;       //for retreiving what the program sends via the normal stream
    std::istream* _PERR;      //for retreiving what the program sends via the error stream
    pid_t _PID{NULL};

    __gnu_cxx::stdio_filebuf<char>* fbOUT;
    __gnu_cxx::stdio_filebuf<char>* fbIN;
    __gnu_cxx::stdio_filebuf<char>* fbERR;
};

class pipe : private _pipe{     //Use this, starts a process with the specified command, and creates ostream and isteram objects. Destroying the object also kills the process, if its not dead yet.
public:
    pipe(std::string process) : _pipe(process){}
    ~pipe(){}
    std::ostream& POUT{*_POUT};      //for sending data to the program
    std::istream& PIN {*_PIN} ;      //for retreiving what the program sends via the normal stream
    std::istream& PERR{*_PERR};      //for retreiving what the program sends via the error stream

    const pid_t& PID{_PID};
};



/* Here we put some specific programs we use often
 *
 *
 */

class gnuplot : public pipe{
public:
    gnuplot() : pipe("/usr/bin/gnuplot"){}
    ~gnuplot(){
        POUT<<"\nexit\n";
        POUT.flush();
    }
};





#endif // PIPE_H

