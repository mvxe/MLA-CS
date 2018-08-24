#include "mako.h"

void CamObserver::CameraListChanged ( AVT::VmbAPI::CameraPtr pCam , AVT::VmbAPI::UpdateTriggerType reason )
{
    //NOTE: Apparently, reason is always UpdateTriggerPluggedOut, regardles of type of the actual event. Must be a bug in the api. I apply a workaround.
    //cam.Close() and cam.Open() triggers this, so I use MVM_ignore flag
    if(sw.MVM_ignore.get()) sw.MVM_ignore.set(false);
    else sw.MAKO_list.set(true);
    sw.MAKO_reco.set(true);
}

FrameObserver::FrameObserver(AVT::VmbAPI::CameraPtr pCamera, std::queue<cv::Mat*>* ptr_queue) : IFrameObserver(pCamera), ptr_queue(ptr_queue){}

int frame=0;
void FrameObserver::FrameReceived(const AVT::VmbAPI::FramePtr pFrame){
    if ((frame%168)==0)std::cerr<<"ack\n";
    frame++;

    VmbUint32_t xsize,ysize;
    VmbPixelFormatType form;
    pFrame->GetWidth(xsize);
    pFrame->GetHeight(ysize);
    pFrame->GetPixelFormat(form);
    if (imgfor::ocv_type_get(form).ocv_type==-1){
        std::cerr<<"The PixelFormat type is not supported(this should have been prevented by GUI selector)! Skipping frame.\n";
        m_pCamera -> QueueFrame ( pFrame ); return;
    }
    if (ptr_queue->front()->rows!=xsize || ptr_queue->front()->cols!=ysize){    //the allocated matrix is of the wrong size/type
        ptr_queue->front()->release();
        *(ptr_queue->front())=cv::Mat(xsize, ysize, imgfor::ocv_type_get(form).ocv_type);
        //std::cerr<<"typename: "<<imgfor::ocv_type_get(form).vmb_name<<" ,  ocv type: "<<imgfor::ocv_type_get(form).ocv_type<<"\n";
    }
    VmbUint32_t bufsize;
    pFrame->GetBufferSize(bufsize);
    if (bufsize>(ptr_queue->front()->dataend - ptr_queue->front()->datastart)){
        std::cerr<<"Image buffer from Vimba too large(this should not be possible) "<< bufsize <<"  "<<(ptr_queue->front()->dataend - ptr_queue->front()->datastart) <<" ! Skipping frame.\n";
        m_pCamera -> QueueFrame ( pFrame ); return;
    }

    pFrame->GetImage(ptr_queue->front()->data);
    if (imgfor::ocv_type_get(form).ccc!=(cv::ColorConversionCodes)-1) cvtColor(*ptr_queue->front(), *ptr_queue->front(), imgfor::ocv_type_get(form).ccc);   //color conversion if img is not monochrome or bgr
    ptr_queue->push(ptr_queue->front());    //the one at the front
    ptr_queue->pop();                       //is moved to the end

    m_pCamera -> QueueFrame ( pFrame );
}


