#include "containers.h"

/*     FILE SAVE CONTAINER - CC_SAVE        */

    // see mutex_containers_impl.h

/*     MUXED STRING CONTAINER FOR XPS QUEUE        */

xps_ret::xps_ret(): done(false) {v.retval=-9999;}
bool xps_ret::block_till_done(){
    for(;;){
        {std::lock_guard<std::mutex>lock(mx);
            if(done) break;}
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
    return (v.retval!=0);
}
bool xps_ret::check_if_done(){
    std::lock_guard<std::mutex>lock(mx);      //just as a ref, the mutex is unlocked after lk destructs, which happens AFTER the return var is copied so this is thread safe
    return done;
}
void xps_ret::set_value(std::string val){
    std::lock_guard<std::mutex>lock(mx);
    if(done) return;    //to prevent multiple access, just in case
    v.retstr=val;
    std::stringstream tmp(v.retstr);
    tmp>>v.retval;
    done=true;
}
