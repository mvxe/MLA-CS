#include "sharedvars.h"

#define _CODE0_ "fo@}=Z]!5W5CW/jL"  //hardcoded random codes
#define _NCHC0_ 16                  //num of characters in code
#define _CODE1_ "@{5d&afkyAR8>a&V"
#define _NCHC1_ 16
//the config is saved into a plain text file in format VAR1NAME_CODE0_VAR1VALUE_CODE1_VAR2NAME_CODE0_... and so on without newlines

sharedvars sw;    //only one istance of the object is needed
sharedvarsba::sharedvarsba(){  //at program start we read from file
    std::ifstream ifile;
    ifile.open ("mla_cs.conf");
    buffer << ifile.rdbuf();
    ifile.close();
    int iter=buffer.str().find(_CODE1_,0), iter2, iter3;
    if (iter!=std::string::npos)
        for (;;iter=iter3){
            iter2 = buffer.str().find(_CODE0_,iter+_NCHC0_);
            if (iter2 == std::string::npos) break;
            iter3 = buffer.str().find(_CODE1_,iter2+_NCHC1_);
            if (iter3 == std::string::npos) break;
            var.push_back({buffer.str().substr(iter+_NCHC0_,iter2-iter-_NCHC0_),buffer.str().substr(iter2+_NCHC1_,iter3-iter2-_NCHC1_)});
        }
}
sharedvarsba::~sharedvarsba(){  //at program end we write to file
    std::ofstream ofile;
    ofile.open ("mla_cs.conf.backup",std::ofstream::trunc); //making backup of old config
    ofile << buffer.str();
    ofile.close();

    ofile.open ("mla_cs.conf",std::ofstream::trunc);
    ofile << _CODE1_;
    for (int i=0;i!=var.size();i++)
        ofile << var[i].strname << _CODE0_ << var[i].strval << _CODE1_;
    ofile.close();
}
