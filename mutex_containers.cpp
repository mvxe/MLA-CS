#include "mutex_containers.h"

/*########## mxvar_whatever ##########*/

mxvar_ip::mxvar_ip(std::mutex *mxn, std::string initial) : mxvar<std::string>(mxn, initial){
    if (check(initial)) err(initial);
}

bool mxvar_ip::check(std::string nvar){
    int n = 0;
    int nums[4];
    for (int i=0;i!=nvar.size();i++){
        if (nvar[i]=='.') n++;
        else if (nvar[i]>='0' && nvar[i]<='9');
        else return false;      //character not a number or dot
    }
    if (n!=3 || sscanf(nvar.c_str(),"%d.%d.%d.%d",&nums[0],&nums[1],&nums[2],&nums[3])!=4) return true;     //must be three dots and three numbers
    for (int i=0;i!=4;i++) if (nums[i]<0 || nums[i]>255) return true;
    return false;
}

/*####*/

mxvar_port::mxvar_port(std::mutex *mxn, int initial) : mxvar<int>(mxn, initial){
    if (check(initial)) err(initial);
}

bool mxvar_port::check(int nvar){
    if (nvar<0 || nvar>65535) return true;
    return false;
}

