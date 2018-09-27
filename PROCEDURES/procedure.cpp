#include "PROCEDURES/procedure.h"

procedure::procedure(){}
procedure::~procedure(){}
bool procedure::startup(){return false;}
void procedure::cleanup(){}
void procedure::run(){
    if (startup()) {cleanup();return;}
    for (;;){
        if(end || work()){
            cleanup();
            end=true;
            done=true;
            return;
        }
    }
}
