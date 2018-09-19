#include "PROCEDURES/procedure.h"

procedure::procedure(){}
procedure::~procedure(){}
bool procedure::init(){return false;}
void procedure::cleanup(){}
void procedure::run(){
    if (init()) {cleanup();return;}
    for (;;){
        if(end || work()){
            cleanup();
            end = true;
            return;
        }
    }
}
