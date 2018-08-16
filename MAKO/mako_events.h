#ifndef MAKO_EVENTS_H
#define MAKO_EVENTS_H


class CamObserver : public AVT::VmbAPI::ICameraListObserver{
public :
    void CameraListChanged ( AVT::VmbAPI::CameraPtr pCam , AVT::VmbAPI::UpdateTriggerType reason );
};
#endif // MAKO_EVENTS_H
