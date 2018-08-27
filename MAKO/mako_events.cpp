#include "mako.h"

void CamObserver::CameraListChanged ( AVT::VmbAPI::CameraPtr pCam , AVT::VmbAPI::UpdateTriggerType reason )
{
    //NOTE: Apparently, reason is always UpdateTriggerPluggedOut, regardles of type of the actual event. Must be a bug in the api. I apply a workaround.
    //cam.Close() and cam.Open() triggers this, so I use MVM_ignore flag
    if(sw.MVM_ignore.get()) sw.MVM_ignore.set(false);
    else sw.MAKO_list.set(true);
    sw.MAKO_reco.set(true);
}

FrameObserver::FrameObserver(AVT::VmbAPI::CameraPtr pCamera, FQsPC *FQsPCcam) : IFrameObserver(pCamera), FQsPCcam(FQsPCcam){}

void FrameObserver::FrameReceived(const AVT::VmbAPI::FramePtr pFrame){
    VmbUint32_t xsize,ysize;
    VmbPixelFormatType form;
    pFrame->GetWidth(xsize);
    pFrame->GetHeight(ysize);
    pFrame->GetPixelFormat(form);
    if (imgfor::ocv_type_get(form).ocv_type==-1){
        std::cerr<<"The PixelFormat type is not supported(this should have been prevented by GUI selector)! Skipping frame.\n";
        m_pCamera -> QueueFrame ( pFrame ); return;
    }
    cv::Mat* freeMat = FQsPCcam->getAFreeMatPtr();
    if (freeMat->rows!=ysize || freeMat->cols!=xsize){    //the allocated matrix is of the wrong size/type
        freeMat->release();
        *(freeMat)=cv::Mat(ysize, xsize, imgfor::ocv_type_get(form).ocv_type);
        //std::cerr<<"typename: "<<imgfor::ocv_type_get(form).vmb_name<<" ,  ocv type: "<<imgfor::ocv_type_get(form).ocv_type<<"\n";
    }
    VmbUint32_t bufsize;
    pFrame->GetBufferSize(bufsize);
    if (bufsize!=(freeMat->dataend - freeMat->datastart)){
        std::cerr<<"Image buffer from Vimba of wrong size(this should not be possible) "<< bufsize <<"  "<<(freeMat->dataend - freeMat->datastart) <<" ! Skipping frame.\n";
        m_pCamera -> QueueFrame ( pFrame ); return;
    }

    pFrame->GetImage(freeMat->data);
    if (imgfor::ocv_type_get(form).ccc!=(cv::ColorConversionCodes)-1) cvtColor(*freeMat, *freeMat, imgfor::ocv_type_get(form).ccc);   //color conversion if img is not monochrome or bgr
    FQsPCcam->enqueueMat();

    //std::cerr<<"Total mat number: "<<FQsPCcam->getMatNumber()<<" ,Full mat number: "<<FQsPCcam->getFullNumber()<<"\n";
    m_pCamera -> QueueFrame ( pFrame );
}


