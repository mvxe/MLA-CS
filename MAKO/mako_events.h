#ifndef MAKO_EVENTS_H
#define MAKO_EVENTS_H


class CamObserver : public AVT::VmbAPI::ICameraListObserver{
public :
    void CameraListChanged(AVT::VmbAPI::CameraPtr pCam ,AVT::VmbAPI::UpdateTriggerType reason);
};

class FrameObserver : public AVT::VmbAPI::IFrameObserver{
public :
    FrameObserver(AVT::VmbAPI::CameraPtr pCamera, std::queue<cv::Mat*>* ptr_queue);
    void FrameReceived(const AVT::VmbAPI::FramePtr pFrame);
private:
    std::queue<cv::Mat*>* ptr_queue;
};

#endif // MAKO_EVENTS_H

