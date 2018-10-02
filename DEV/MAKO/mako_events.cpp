#include "DEV/MAKO/mako.h"

void CamObserver::CameraListChanged ( AVT::VmbAPI::CameraPtr pCam , AVT::VmbAPI::UpdateTriggerType reason )
{
    //NOTE: Apparently, reason is always UpdateTriggerPluggedOut, regardles of type of the actual event. Must be a bug in the api. I apply a workaround.
    //cam.Close() and cam.Open() triggers this, so I use MVM_ignore flag
    if(go.pMAKO->MVM_ignore) go.pMAKO->MVM_ignore=false;
    else go.pMAKO->MVM_list=true;                        //TODO now that frame queues are implemented, this is broken (program freezes on camera disconnected)
    go.pMAKO->MAKO_reco=true;
}

FrameObserver::FrameObserver(AVT::VmbAPI::CameraPtr pCamera, FQsPC *FQsPCcam, camobj *Camobj) : IFrameObserver(pCamera), FQsPCcam(FQsPCcam), Camobj(Camobj){}

//THERE IS NO ANCILLIARY DATA FOR MAKO CAMERA, TRYING TO ACCESS IT WILL SEGFAULT
std::mutex FrameObserver::prot;
void FrameObserver::FrameReceived(const AVT::VmbAPI::FramePtr pFrame){
    VmbUint32_t xsize,ysize;
    VmbPixelFormatType form;
    pFrame->GetWidth(xsize);
    pFrame->GetHeight(ysize);
    pFrame->GetPixelFormat(form);
    if (imgfor::ocv_type_get(form).ocv_type==-1){
        std::cerr<<"The PixelFormat type is not supported(this should have been prevented by GUI selector)! Skipping frame.\n";
        m_pCamera->QueueFrame ( pFrame ); return;
    }
  std::lock_guard<std::mutex>lock(prot);
    cv::Mat* freeMat = FQsPCcam->getAFreeMatPtr();

    if (freeMat->rows!=ysize || freeMat->cols!=xsize){    //the allocated matrix is of the wrong size/type
        delete freeMat;
        freeMat=new cv::Mat(ysize, xsize, imgfor::ocv_type_get(form).ocv_type);
        //std::cerr<<"typename: "<<imgfor::ocv_type_get(form).vmb_name<<" ,  ocv type: "<<imgfor::ocv_type_get(form).ocv_type<<"\n";
    }
    VmbUint32_t bufsize;
    pFrame->GetImageSize(bufsize);
    if (bufsize!=(freeMat->dataend - freeMat->datastart)){
        std::cerr<<"Image buffer from Vimba of wrong size(this should not be possible) "<< bufsize <<"  "<<(freeMat->dataend - freeMat->datastart) <<" ! Skipping frame.\n";
        m_pCamera->QueueFrame ( pFrame ); return;
    }

    pFrame->GetImage(freeMat->data);
    Camobj->linkFrameToMat(pFrame, freeMat);

    if (imgfor::ocv_type_get(form).ccc!=(cv::ColorConversionCodes)-1) {std::cerr<<"converted color\n"; cvtColor(*freeMat, *freeMat, imgfor::ocv_type_get(form).ccc);}   //color conversion if img is not monochrome or bgr

    VmbUint64_t timestamp;
    pFrame->GetTimestamp(timestamp);
    FQsPCcam->enqueueMat(timestamp);
    freeMat = FQsPCcam->getAFreeMatPtr();
    if (freeMat!=nullptr)
        if (Camobj->requeueFrame(freeMat)) return;
    Camobj->newFrame();
}


