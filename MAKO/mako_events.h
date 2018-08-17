#ifndef MAKO_EVENTS_H
#define MAKO_EVENTS_H


class CamObserver : public AVT::VmbAPI::ICameraListObserver{
public :
    void CameraListChanged(AVT::VmbAPI::CameraPtr pCam ,AVT::VmbAPI::UpdateTriggerType reason);
};

class FrameObserver : public AVT::VmbAPI::IFrameObserver{
public :
    FrameObserver(AVT::VmbAPI::CameraPtr pCamera);
    void FrameReceived(const AVT::VmbAPI::FramePtr pFrame);
};

#endif // MAKO_EVENTS_H

