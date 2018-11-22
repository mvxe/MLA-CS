#include "UTIL/containers.h"

/*########## exec_dat ##########*/

bool exec_ret::block_till_done(){
    for(;;){
        {std::lock_guard<std::mutex>lock(mx);
            if(done) break;}
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
    std::lock_guard<std::mutex>lock(mx);
    return (v.retval!=0);
}
bool exec_ret::check_if_done(){
    std::lock_guard<std::mutex>lock(mx);      //just as a ref, the mutex is unlocked after lk destructs, which happens AFTER the return var is copied so this is thread safe
    return done;
}
void exec_ret::reset(){
    std::lock_guard<std::mutex>lock(mx);
    done=false;
    v.retval=-9999;
}
void exec_ret::set_value(std::string val){
    std::lock_guard<std::mutex>lock(mx);
    if(done) return;    //to prevent multiple access, just in case
    v.retstr=val;
    std::stringstream tmp(v.retstr);
    tmp>>v.retval;
    done=true;
}

/*########## som functions for internal use ##########*/
#include <globals.h>    //here to prevent excessive interlinking
void _containers_internal::quit(){
    go.quit();
}

        /*########## tsvar_save_ip ##########*/

        bool tsvar_save_ip::check(std::string nvar){
            int n = 0;
            int nums[4];
            for (int i=0;i!=nvar.size();i++){
                if (nvar[i]=='.') n++;
                else if (nvar[i]>='0' && nvar[i]<='9');
                else {      //character not a number or dot: let it go, maybe its a hostname}
                    is_name=true;
                    return false;
                }
            }
            if (n!=3 || sscanf(nvar.c_str(),"%d.%d.%d.%d",&nums[0],&nums[1],&nums[2],&nums[3])!=4) return true;     //must be three dots and three numbers
            for (int i=0;i!=4;i++) if (nums[i]<0 || nums[i]>255) return true;
            is_name=false;
            return false;
        }

        /*########## tsvar_save_port ##########*/

        bool tsvar_save_port::check(int nvar){
            if (nvar<0 || nvar>65535) return true;
            return false;
        }


/*########## tsbool ##########
bool container with expiration time*/

tsbool::tsbool(std::mutex *mxn) : mx(mxn) {time(&mf);}
void tsbool::set(bool nvar, double expt){
    std::lock_guard<std::mutex>lock(*mx);
    exp_time=expt;
    var = nvar;
    time(&mf);
}
bool tsbool::get(){
    std::lock_guard<std::mutex>lock(*mx);
    if (difftime(time(NULL),mf)>exp_time) var = false;
    return var;
}

/*########## cc_save ##########*/

template <> cc_save<std::string>::cc_save(std::string& var, std::string initial, _savelst* vec, std::string name): val(var){            //we specialize this one because stringstream only puts the first word into the string, ignoring others after the first whitespace
    val=initial;
    if(vec!=nullptr){
        for (int i=0;i!=vec->size();i++){
            if (name.compare((*vec)[i].strname)==0){
                val=(*vec)[i].strval;
                tfvec=&(*vec)[i];
                return;
            }
        }
        vec->push_back({name,""});  //if it doesnt exist
        tfvec=&vec->back();
    }
}
