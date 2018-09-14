#include "mako.h"

camobj::camobj(MAKO *cobj, std::string strID) : camobj_config(strID), cobj(cobj), ID(&mkmx,strID,&go.config.save,strID){}

void camobj::start(){
    VMBo = AVT::VmbAPI::IFrameObserverPtr(new FrameObserver(cam.ptr, &FQsPCcam));

    VmbInt64_t nPLS=wfun::get<VmbInt64_t>(cam.ptr,"PayloadSize");
    Xsize=wfun::get<VmbInt64_t>(cam.ptr,"Width");
    Ysize=wfun::get<VmbInt64_t>(cam.ptr,"Height");
    format_enum=wfun::get<VmbInt64_t>(cam.ptr,"PixelFormat");

/* TODO implement this in gui later on*/
    AVT::VmbAPI::FeaturePtrVector features;
    cam.ptr->GetFeatures(features);
    std::string fname[5];
    bool rw[2];
    for (int i=0;i!=features.size();i++){
        features[i]->GetName(fname[0]);
        features[i]->GetUnit(fname[1]);
        features[i]->GetRepresentation(fname[2]);
        features[i]->GetToolTip(fname[3]);
        features[i]->GetDescription(fname[4]);
        features[i]->IsReadable(rw[0]);
        features[i]->IsWritable(rw[1]);
        //std::cerr<<"feature name: "<<fname[0]<<"\n  Unit: "<<fname[1]<<"\n  Representation: "<<fname[2]<<"\n  ToolTip: "<<fname[3]<<"\n  Description: "<<fname[4];
        //std::cerr<<"\n  R/W: "<<rw[0]<<"/"<<rw[1]<<"\n\n";
    }
/*#####################################*/

    std::cerr<<"Xsize="<<Xsize<<"\n";
    std::cerr<<"Ysize="<<Ysize<<"\n";
    std::cerr<<"format="<<format_enum<<"\n";

    //wfun::set<double>(cam.ptr,"ExposureTime",1000.);
    wfun::set<double>(cam.ptr,"ExposureTime",expo.get());
    expo.set(wfun::get<double>(cam.ptr,"ExposureTime"));
    std::cerr<<"exposure(us)="<<expo.get()<<"\n";

    ackFPS=wfun::get<double>(cam.ptr,"AcquisitionFrameRate");
    FQsPCcam.setCamFPS(ackFPS);
    std::cerr<<"ackFPS="<<ackFPS<<"\n";

    for (int i=0; i!=VMBframes.size();i++){
        VMBframes[i].reset(new AVT::VmbAPI::Frame(nPLS));
        VMBframes[i]->RegisterObserver(VMBo);
        cam.ptr->AnnounceFrame(VMBframes[i]);
    }
    if (cam.ptr->StartCapture()!=VmbErrorSuccess) {end(); return;}
    for (int i=0; i!=VMBframes.size();i++) cam.ptr->QueueFrame(VMBframes[i]);
}
void camobj::work(){
    std::lock_guard<std::mutex>lock(mtx);
    if (!_connected){
        ackstatus=false;
        return;
    }
    bool doack=false;
    if (FQsPCcam.isThereInterest()){
        if (!ackstatus) ackstatus=(wfun::run(cam.ptr,"AcquisitionStart")==VmbErrorSuccess);
        doack=ackstatus;
    }
    if(doack==false && ackstatus==true){
        wfun::run(cam.ptr,"AcquisitionStop");
        ackstatus=false;
    }
}
void camobj::end(){
    std::lock_guard<std::mutex>lock(mtx);
    _end();
}
void camobj::_end(){
    if (ackstatus) wfun::run(cam.ptr,"AcquisitionStop");
    ackstatus=false;
    cam.ptr->EndCapture();
    cam.ptr->FlushQueue();
    cam.ptr->RevokeAllFrames();
    for (int i=0; i!=VMBframes.size();i++){
        VMBframes[i]->UnregisterObserver();
        VMBframes[i].reset();
    }
    VMBo.reset();
}
void camobj::con_cam(bool ch){
    std::lock_guard<std::mutex>lock(mtx);
    if (cam.ptr!=nullptr){
        if (ch && (cam.ID == ID.get())) return;
        cobj->MVM_ignore=true;
        _end();
        cam.ptr->Close();
        cam.ptr.reset();
    }
    if (ID.get()!="none"){
        bool found = false;
        for (int i=0;i!=cobj->cams.size();i++){
            if (ID.get()==cobj->cams[i].ID){
                cam = cobj->cams[i];
                found = true;
                break;
            }
        }
        if (found){
            cobj->MVM_ignore=true;
            if (cam.ptr->Open(VmbAccessModeFull) == VmbErrorSuccess){
                _connected=true;
                start();
                return;
            }else{
                cobj->MVM_ignore=false;
                cam.ptr.reset();
            }
        }
    }
    _connected=false;
}
