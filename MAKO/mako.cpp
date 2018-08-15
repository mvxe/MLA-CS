#include "mako.h"

MAKO::MAKO() : vsys( AVT::VmbAPI::VimbaSystem::GetInstance() ){
    VmbErrorType errc = vsys.Startup();
    if (errc!=VmbErrorSuccess){
        std::cerr<<"Vimba system startup error: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
}
MAKO::~MAKO(){
    if (vsys.Shutdown()!=VmbErrorSuccess){
        std::cerr<<"Vimba system shutdown error.\n";
        exit (EXIT_FAILURE);
    }
}

void MAKO::run(){    //this is the MAKO thread loop
    for (;;){
        refresh_cams();
        std::this_thread::sleep_for (std::chrono::milliseconds(1000));

        if(sw.MAKO_end.get()){
            //TODO cleanup
            std::cout<<"MAKO thread exited.\n";
            sw.MAKO_end.set(false);
            return;
        }
    }
}

void MAKO::refresh_cams(){
    cams.clear();
    bool found_iuScope = false;
    VmbErrorType errc = vsys.GetCameras(cameras);
    if (errc == VmbErrorSuccess){
        for (int i=0;i!=cameras.size();i++){
            cams.emplace_back();
            cams.back().ptr = cameras[i];
            cams.back().ptr->GetID(cams.back().ID);
            if (cams.back().ID.compare(sw.iuScopeID.get())==0) found_iuScope=true;
        }
        if(sw.MAKO_cam_desc.get()==nullptr){
            std::vector<_dcams> *dcams = new std::vector<_dcams>();
            for (int i=0;i!=cameras.size();i++){
                std::string temp;
                dcams->emplace_back();
                cams.back().ptr->GetID(temp); dcams->back().ID=temp;
                cams.back().ptr->GetName(temp); dcams->back().description+="Name: "+temp;
                cams.back().ptr->GetModel(temp); dcams->back().description+="\nModel: "+temp;
                cams.back().ptr->GetSerialNumber(temp); dcams->back().description+="\nSerialNumber: "+temp;
                cams.back().ptr->GetInterfaceID(temp); dcams->back().description+="\nInterfaceID: "+temp;
            }
            sw.MAKO_cam_desc.set(dcams);
        }
    }
    else {
        std::cerr<<"Vimba error getting cameras: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
    sw.iuScope_connected.set(found_iuScope);
}
