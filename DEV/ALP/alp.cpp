#include "alp.h"
                    //TODO clean up these functions a bit
ALP::ALP(){}
ALP::~ALP(){}

int32_t ALP::AlpDevAlloc(int32_t DeviceNum,int32_t InitFlag, ALP_ID* DeviceIdPtr){
    int32_t toSend[3]={ 0,DeviceNum,InitFlag};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *DeviceIdPtr=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpDevHalt(ALP_ID DeviceId){
    int32_t toSend[2]={ 1,(int32_t)DeviceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpDevFree(ALP_ID DeviceId){
    int32_t toSend[2]={ 2,(int32_t)DeviceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpDevControl(ALP_ID DeviceId, int32_t ControlType, int32_t ControlValue){
    int32_t toSend[4]={ 3,(int32_t)DeviceId,ControlType,ControlValue};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpDevControlEx(ALP_ID DeviceId, int32_t ControlType, tAlpDynSynchOutGate *UserStructPtr){
    int32_t toSend[3]={ 4,(int32_t)DeviceId,ControlType};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::write(UserStructPtr,sizeof(*UserStructPtr))!=sizeof(*UserStructPtr)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpDevInquire(ALP_ID DeviceId, int32_t InquireType, int32_t *UserVarPtr){
    int32_t toSend[3]={ 5,(int32_t)DeviceId,InquireType};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *UserVarPtr=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpSeqAlloc(ALP_ID DeviceId, int32_t BitPlanes, int32_t PicNum,  ALP_ID *SequenceIdPtr){
    int32_t toSend[4]={ 6,(int32_t)DeviceId,BitPlanes,PicNum};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *SequenceIdPtr=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpSeqFree(ALP_ID DeviceId, ALP_ID SequenceId){
    int32_t toSend[3]={ 7,(int32_t)DeviceId,(int32_t)SequenceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpSeqControl(ALP_ID DeviceId, ALP_ID SequenceId,  int32_t ControlType, int32_t ControlValue){
    int32_t toSend[5]={ 8,(int32_t)DeviceId,(int32_t)SequenceId,ControlType,ControlValue};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpSeqTiming(ALP_ID DeviceId, ALP_ID SequenceId,  int32_t IlluminateTime, int32_t PictureTime, int32_t SynchDelay, int32_t SynchPulseWidth, int32_t TriggerInDelay){
    int32_t toSend[8]={ 9,(int32_t)DeviceId,(int32_t)SequenceId,IlluminateTime,PictureTime,SynchDelay,SynchPulseWidth,TriggerInDelay};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpSeqInquire(ALP_ID DeviceId, ALP_ID SequenceId,  int32_t InquireType, int32_t *UserVarPtr){
    int32_t toSend[4]={10,(int32_t)DeviceId,(int32_t)SequenceId,InquireType};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *UserVarPtr=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpSeqPut(ALP_ID DeviceId, ALP_ID SequenceId, int32_t PicOffset, int32_t PicLoad, void *UserArrayPtr, size_t UserArraySize){
    int32_t toSend[6]={11,(int32_t)DeviceId,(int32_t)SequenceId,PicOffset,PicLoad,(int32_t)UserArraySize};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::write(UserArrayPtr,UserArraySize)!=UserArraySize) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjStart(ALP_ID DeviceId, ALP_ID SequenceId){
    int32_t toSend[3]={12,(int32_t)DeviceId,(int32_t)SequenceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjStartCont(ALP_ID DeviceId, ALP_ID SequenceId){
    int32_t toSend[3]={13,(int32_t)DeviceId,(int32_t)SequenceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjHalt(ALP_ID DeviceId){
    int32_t toSend[2]={14,(int32_t)DeviceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjWait(ALP_ID DeviceId){
    int32_t toSend[2]={15,(int32_t)DeviceId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjControl(ALP_ID DeviceId, int32_t ControlType, int32_t ControlValue){
    int32_t toSend[4]={16,(int32_t)DeviceId,ControlType,ControlValue};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjControlEx(ALP_ID DeviceId, int32_t ControlType, tFlutWrite *pUserStructPtr ){
    int32_t toSend[3]={17,(int32_t)DeviceId,ControlType};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::write(pUserStructPtr,sizeof(*pUserStructPtr))!=sizeof(*pUserStructPtr)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpProjInquire(ALP_ID DeviceId, int32_t InquireType, int32_t *UserVarPtr){
    int32_t toSend[3]={18,(int32_t)DeviceId,InquireType};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *UserVarPtr=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpProjInquireEx(ALP_ID DeviceId, int32_t InquireType, tAlpProjProgress *UserStructPtr ){
    int32_t toSend[3]={19,(int32_t)DeviceId,InquireType};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    if (TCP_con::bread(UserStructPtr,sizeof(*UserStructPtr))!=sizeof(*UserStructPtr)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpLedAlloc(ALP_ID DeviceId, int32_t LedType, tAlpHldPt120AllocParams *UserStructPtr, ALP_ID *LedId ){
    int32_t toSend[3]={20,(int32_t)DeviceId,LedType};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::write(UserStructPtr,sizeof(*UserStructPtr))!=sizeof(*UserStructPtr)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *LedId=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpLedFree(ALP_ID DeviceId, ALP_ID LedId ){
    int32_t toSend[3]={21,(int32_t)DeviceId,(int32_t)LedId};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpLedControl(ALP_ID DeviceId, ALP_ID LedId, int32_t ControlType, int32_t Value ){
    int32_t toSend[5]={22,(int32_t)DeviceId,(int32_t)LedId,ControlType,Value};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    return toRecv[0];
}
int32_t ALP::AlpLedInquire(ALP_ID DeviceId, ALP_ID LedId, int32_t InquireType, int32_t *UserVarPtr ){
    int32_t toSend[4]={23,(int32_t)DeviceId,(int32_t)LedId,InquireType};
    int32_t toRecv[2];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    *UserVarPtr=toRecv[1];
    return toRecv[0];
}
int32_t ALP::AlpLedInquireEx(ALP_ID DeviceId, ALP_ID LedId, int32_t InquireType, tAlpHldPt120AllocParams *UserStructPtr){
    int32_t toSend[4]={24,(int32_t)DeviceId,(int32_t)LedId,InquireType};
    int32_t toRecv[1];
    if (TCP_con::write(toSend,sizeof(toSend))!=sizeof(toSend)) return RET_ERROR;
    if (TCP_con::bread(toRecv,sizeof(toRecv))!=sizeof(toRecv)) return RET_ERROR;
    if (TCP_con::bread(UserStructPtr,sizeof(*UserStructPtr))!=sizeof(*UserStructPtr)) return RET_ERROR;
    return toRecv[0];
}

