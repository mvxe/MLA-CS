#include "DEV/RPTY/rpty.h"
#include "globals.h"

RPTY::RPTY(){}
RPTY::~RPTY(){}

void RPTY::run(){
    std::string tmp;
    for (;;){
        while (!connected && !end){
                //resolving...
            std::string resname;    //this is when user enters hostname instead of ip
            if (resolve(IP.get(), port.get(), &resname)){
                resname = "cannot resolve this hostname";
                std::this_thread::sleep_for (std::chrono::milliseconds(keepalive.get()));
            }
            else{
                    //connecting...
                connect(keepalive.get());
                if (connected) {/*TODO RPTY init*/}
            }
            IP.resolved.set(resname);
        }
        if(connected && (IP.changed() || port.changed())) {/*TODO RPTY disco*/ disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected); /*TODO: RPTY do work here*/

        if(end){
            if (connected){
                /*TODO RPTY disco*/
                disconnect();
            }
            std::cout<<"RPTY thread exited.\n";
            done=true;
            return;
        }
    }
}

int RPTY::F2A_read(unsigned char queue, uint32_t *data, uint32_t size4){
    uint32_t command[3]={0,queue,size4};
    TCP_con::write(command,12);
    uint32_t index_read=0;
    ssize_t ret=0;
    while(index_read<size4){
        ret=TCP_con::read(data+index_read,4*(size4-index_read))/4;
        if(ret<0) return -1;
        index_read+=ret;
    }
    return index_read;  //allways should =size4 else infinite loop, TODO fix maybe?
}
int RPTY::A2F_write(unsigned char queue, uint32_t *data, uint32_t size4){
    uint32_t command[3]={1,queue,size4};
    TCP_con::write(command,12);
    return TCP_con::write(data,4*size4);
}
int RPTY::getNum(getNumType statID, unsigned char queue){
    uint32_t command[2]={2,queue};
    command[0]+=(int)statID;
    TCP_con::write(command,8);
    uint32_t nret; int ret;
    do nret=TCP_con::read(&ret,4);
    while (nret<4);
    return ret;
}
int RPTY::trig(unsigned char queue){
    uint32_t command[2]={8,(uint32_t)(queue)};
    return TCP_con::write(command,8);
}
int RPTY::FIFOreset(unsigned char A2Fqueues, unsigned char F2Aqueues){
    uint32_t command[2]={9,(uint32_t)((A2Fqueues&0xF)&((F2Aqueues&0xF)<<4))};
    return TCP_con::write(command,8);
}
int RPTY::FIFOreset(){
    uint32_t command[2]={9,0x100};
    return TCP_con::write(command,8);
}
int RPTY::PIDreset(unsigned char PIDN){
    uint32_t command[2]={100,(uint32_t)(PIDN&0x3)};
    return TCP_con::write(command,8);
}
int RPTY::PIDreset(){
    uint32_t command[2]={100,0x4};
    return TCP_con::write(command,8);
}
