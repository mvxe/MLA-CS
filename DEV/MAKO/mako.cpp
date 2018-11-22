#include "DEV/MAKO/mako.h"
#include "globals.h"

MAKO::MAKO(){
    camobj::cobj=this;
    for(int i=0;i!=_CAM_NUM;i++)
        _c[i].ptr=new camobj(_c[i].cname);
    cam_desc.set(new std::vector<_dcams>());
    arv_g_type_init();
}
MAKO::~MAKO(){
    for(int i=0;i!=_CAM_NUM;i++) delete _c[i].ptr;
    arv_shutdown();
}

void MAKO::run(){    //this is the MAKO thread loop
    for (;;){
        if (MVM_list) {
            list_cams();
            MVM_list=false;
        }

        for(int i=0;i!=_CAM_NUM;i++)
            _c[i].ptr->work();
        std::this_thread::sleep_for (std::chrono::milliseconds(1));               //TODO fix this delay with timers

        if(end){
            for(int i=0;i!=_CAM_NUM;i++)
                if (_c[i].ptr->cam!=NULL)
                    _c[i].ptr->end();
            std::cout<<"MAKO thread exited.\n";
            done=true;
            return;
        }
    }
}

void MAKO::list_cams(){
    arv_update_device_list();
    int Ncams=arv_get_n_devices();
    cam_desc.get()->clear();
    for (int i=0;i!=Ncams;i++){
        cam_desc.get()->emplace_back();
        cam_desc.get()->back().ID=arv_get_device_id(i);
        cam_desc.get()->back().description+="Physical ID: " +std::string(arv_get_device_physical_id(i));
        cam_desc.get()->back().description+="\nModel: "+std::string(arv_get_device_model(i));
        cam_desc.get()->back().description+="\nSerial number: "+std::string(arv_get_device_serial_nbr(i));
        cam_desc.get()->back().description+="\nVendor: "+std::string(arv_get_device_vendor(i));
        cam_desc.get()->back().description+="\nAddress: "+std::string(arv_get_device_address(i));
        cam_desc.get()->back().description+="\nProtocol: "+std::string(arv_get_device_protocol(i));
    }
    cam_desc.get()->emplace_back(_dcams{"none",""});
}
