#include "mako.h"

void CamObserver::CameraListChanged ( AVT::VmbAPI::CameraPtr pCam , AVT::VmbAPI::UpdateTriggerType reason )
{
    //NOTE: Apparently, reason is always UpdateTriggerPluggedOut, regardles of type of the actual event. Must be a bug in the api. I apply a workaround.
    //cam.Close() and cam.Open() triggers this, so I use MVM_ignore flag
    if(sw.MVM_ignore.get()) sw.MVM_ignore.set(false);
    else sw.MAKO_list.set(true);
    sw.MAKO_reco.set(true);
}

FrameObserver::FrameObserver(AVT::VmbAPI::CameraPtr pCamera) : IFrameObserver(pCamera){}

int frame=0;
void FrameObserver::FrameReceived(const AVT::VmbAPI::FramePtr pFrame){

    //code
    if ((frame%168)==0)std::cerr<<"f "<<frame<<"\n";
    frame++;

    m_pCamera -> QueueFrame ( pFrame );
}


