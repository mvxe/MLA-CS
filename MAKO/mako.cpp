#include "mako.h"

MAKO::MAKO() : vsys( AVT::VmbAPI::VimbaSystem::GetInstance() ){
    vsys.Startup();
}
MAKO::~MAKO(){
    vsys.Shutdown();
}

void MAKO::run(){    //this is the MAKO thread loop
    for (;;){
        std::this_thread::sleep_for (std::chrono::milliseconds(1));

        if(MAKO_end.get()){
            //cleanup TODO
            MAKO_end.set(false);
            return;
        }
    }
}
