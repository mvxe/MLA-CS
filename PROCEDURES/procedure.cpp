#include "PROCEDURES/procedure.h"

procedure::procedure(){}
procedure::~procedure(){}
void procedure::cleanup(){}
void procedure::run(){
    for (;;){
        if(end || work()){
            cleanup();
            end = true;
            return;
        }
    }
}
