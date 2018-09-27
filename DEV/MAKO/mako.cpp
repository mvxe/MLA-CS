#include "DEV/MAKO/mako.h"
#include "globals.h"

MAKO::MAKO(){
    for(int i=0;i!=_CAM_NUM;i++)
        _c[i].ptr=new camobj(this,_c[i].cname);
    cam_desc.set(new std::vector<_dcams>());
    VmbErrorType errc = vsys.Startup();
    if (errc!=VmbErrorSuccess){
        std::cerr<<"ERROR: Vimba system startup error: "<<errc<<".\n";
        go.quit();
    }
    errc = vsys.RegisterCameraListObserver(AVT::VmbAPI::ICameraListObserverPtr(new CamObserver()));
    if (errc!=VmbErrorSuccess){
        std::cerr<<"ERROR: Vimba system event error: "<<errc<<".\n";
        go.quit();
    }
}
MAKO::~MAKO(){
    for(int i=0;i!=_CAM_NUM;i++) delete _c[i].ptr;
    if (vsys.Shutdown()!=VmbErrorSuccess){
        std::cerr<<"ERROR: Vimba system shutdown error.\n";
        go.quit();
    }
}

void MAKO::run(){    //this is the MAKO thread loop
    bool ch=true;
    for (;;){
        if (MVM_list) {
            list_cams();
            ch = false;
            MVM_list=false;
        }
        if (MAKO_reco){
            for(int i=0;i!=_CAM_NUM;i++)
                _c[i].ptr->con_cam(ch);
            ch = true;
            MAKO_reco=false;
        }

        for(int i=0;i!=_CAM_NUM;i++)
            _c[i].ptr->work();
        std::this_thread::sleep_for (std::chrono::milliseconds(1));               //TODO fix this delay with timers

        if(end){
            for(int i=0;i!=_CAM_NUM;i++)
                if (_c[i].ptr->cam.ptr!=nullptr)
                    _c[i].ptr->end();
            std::cout<<"MAKO thread exited.\n";
            done=true;
            return;
        }
    }
}

void MAKO::list_cams(){
    cams.clear();
    VmbErrorType errc = vsys.GetCameras(cameras);
    if (errc == VmbErrorSuccess){
        cam_desc.get()->clear();
        for (int i=0;i!=cameras.size();i++){
            cams.emplace_back();
            cams.back().ptr = cameras[i];
            cams.back().ptr->GetID(cams.back().ID);
            std::string temp;
            cam_desc.get()->emplace_back();
            cams.back().ptr->GetID(temp); cam_desc.get()->back().ID=temp;
            cams.back().ptr->GetName(temp); cam_desc.get()->back().description+="Name: "+temp;
            cams.back().ptr->GetModel(temp); cam_desc.get()->back().description+="\nModel: "+temp;
            cams.back().ptr->GetSerialNumber(temp); cam_desc.get()->back().description+="\nSerialNumber: "+temp;
            cams.back().ptr->GetInterfaceID(temp); cam_desc.get()->back().description+="\nInterfaceID: "+temp;
        }
        cam_desc.get()->emplace_back();
        cam_desc.get()->back().ID = "none";
        cam_desc.get()->back().description = "";
    }
    else {
        std::cerr<<"ERROR: Vimba error getting cameras: "<<errc<<".\n";
        go.quit();
    }
}
