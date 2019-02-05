#include "cnc.h"

CNC::CNC() : selected_ID(&mkmx,"none",&go.config.save,"cnc_conid"){
    serial_desc.set(new std::vector<_dserial>());
}
CNC::~CNC(){}

void CNC::run(){
    std::string tmp;
    for (;;){

        if(refreshID){
            std::vector<serial::PortInfo> devices_found=serial::list_ports();
            std::vector<serial::PortInfo>::iterator iter=devices_found.begin();
            serial_desc.get()->clear();
            while(iter!=devices_found.end()){
                serial::PortInfo device=*iter++;
                serial_desc.get()->emplace_back();
                serial_desc.get()->back().ID=device.description;
                serial_desc.get()->back().port=device.port;
                serial_desc.get()->back().description+="Port: "+device.port;
                serial_desc.get()->back().description+="\nHardware_id: "+device.hardware_id;
            }
            serial_desc.get()->emplace_back(_dserial{"none","",""});
            refreshID=false;
        }

        if(!connected || checkID){
            if(selected_ID.get()!=ID){      //we selected a different camera or try to connect to the same one if not connected
                ID="none";
                if (connected) cend();
                if (selected_ID.get()!="none"){ //connects to the new device
                    bool tryv=true;
                    for(int i=0;i!=serial_desc.get()->size();i++)
                        if(selected_ID.get().compare(serial_desc.get()->at(i).ID)==0){
                            port=serial_desc.get()->at(i).port;
                            break;
                        }
                    try {
                        sercon = new serial::Serial(port, baud, serial::Timeout::simpleTimeout(timeout));

                    } catch (std::exception &ex){
                        std::this_thread::sleep_for (std::chrono::milliseconds(100));
                        tryv=false;
                    }
                    if (tryv){
                        if(sercon->isOpen()){
                            _connected=true;
                            ID=selected_ID.get();
                        }
                        else std::this_thread::sleep_for (std::chrono::milliseconds(100));
                    }
                }
            }
            checkID=false;
        }
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected)
            flushQueue();

        if(end){
            if (connected) cend();
            std::cout<<"CNC thread exited.\n";
            done=true;
            return;
        }
    }
}

void CNC::cend(){
    if(sercon->isOpen()){
        sercon->close();
    }
    _connected=false;
}

void CNC::flushQueue(){
    std::string wrstr;
    std::string rdstr;
    int nerr;
    mpq.lock();
    while (!priority_queue.empty()){
        wrstr=priority_queue.front().comm;
        rdstr="";
        mpq.unlock();
        nerr=0;
   xx:  sercon->write(wrstr);
   yy:  rdstr+=sercon->read(65536);

        if(rdstr.find("resend")!=std::string::npos) goto xx;
        else if(rdstr.find("ok\n")!=std::string::npos);
        else {nerr++; if(nerr==1000) {nerr=0; std::cerr<<"cnc: waiting for ok\n"<<rdstr<<"\n";} std::this_thread::sleep_for (std::chrono::milliseconds(1)); goto yy;}

        mpq.lock();
            if(priority_queue.front().ret!=nullptr) priority_queue.front().ret->set_value(wrstr);
            priority_queue.pop();
        mpq.unlock();
        if(true){
            std::cerr<<"SENT: "<<wrstr<<"\n";
            std::cerr<<"RECV: "<<rdstr<<"\n";
        }
        mpq.lock();
    }
    mpq.unlock();
}

