#include "DEV/MAKO/frame_queues.h"

FQsPC::FQsPC(){}
bool FQsPC::isThereInterest(){
    std::lock_guard<std::mutex>lock(userqmx);
    for (int i=0;i!=user_queues.size();i++){
        std::lock_guard<std::mutex>lock(user_queues[i].umx);
        if (user_queues[i].fps!=0) return true;
    }
    return false;
}
cv::Mat* FQsPC::getAFreeMatPtr(){
    std::lock_guard<std::mutex>lock(apicbmx);
    if (mat_ptr_free.empty()){
        reclaim();
        if (mat_ptr_free.empty())
            return nullptr;
    }
    cv::Mat* ret=mat_ptr_free.front();
    mat_ptr_free.pop();
    return ret;
}
void FQsPC::enqueueMat(cv::Mat* mat, unsigned int timestamp){
    unsigned camfps;
    {std::lock_guard<std::mutex>lock(userqmx);
        camfps=fps;}
    std::lock_guard<std::mutex>lock(apicbmx);
    mat_ptr_full.push_front({mat,0,timestamp});
    if ((mat)->rows==0){
        std::cerr<<"ERROR: FQ::enqueueMat says rows are 0. Will not enqueue, returning.\n";
        mat_ptr_full.pop_front();
        return;
    }
    for (int i=0;i!=user_queues.size();i++){
        std::lock_guard<std::mutex>lock(user_queues[i].umx);
        if (user_queues[i].fps!=0){
            if (user_queues[i].maxfr==0 || user_queues[i].full.size()==0) user_queues[i].div=round(camfps/user_queues[i].fps);
            else if (user_queues[i].full.size()<=user_queues[i].maxfr/2 && user_queues[i].div!=round(camfps/user_queues[i].fps)) user_queues[i].div-=1;
            else if (user_queues[i].full.size()>=user_queues[i].maxfr) user_queues[i].div+=1;
            //std::cerr<<"camfps/fps: "<<user_queues[i].div<<"\n";
            if (user_queues[i].i>=user_queues[i].div){
                user_queues[i].i=1;
                user_queues[i].full.push_back({&mat_ptr_full.front().mat,timestamp});
                mat_ptr_full.front().users++;
            }
            else user_queues[i].i++;
        }
    }
    if(mat_ptr_full.front().users==0){
        mat_ptr_full.pop_front();
        mat_ptr_free.push(mat);
    }
    reclaim();
}
unsigned FQsPC::getFreeNumber(){
    return mat_ptr_free.size();
}
unsigned FQsPC::getFullNumber(){
    return mat_ptr_full.size();
}
FQ* FQsPC::getNewFQ(){
    FQ* ret;
    std::lock_guard<std::mutex>lock(userqmx);
    user_queues.emplace_back();
    return &user_queues.back();
}
void FQsPC::deleteFQ(FQ* fq){
    fq->setUserFps(0);
    while(fq->getFullNumber()) fq->freeUserMat();
    //TODO implement destroy queue object
}
void FQsPC::setCamFPS(double nfps){
    std::lock_guard<std::mutex>lock(userqmx);
    fps=nfps;
}
void FQsPC::reclaim(){
    for (int i=0;i!=user_queues.size();i++){
        std::lock_guard<std::mutex>lock(user_queues[i].umx);
        for (std::list<_used>::iterator it=mat_ptr_full.begin();it!=mat_ptr_full.end();it++)
            for (int j=0;j!=user_queues[i].free.size();j++)
                if(user_queues[i].free[j]==(&(*it).mat)){
                    (*it).users--;
                    user_queues[i].free.erase(user_queues[i].free.begin()+j);
                    j--;
                }
    }
    for (std::list<_used>::iterator it=mat_ptr_full.begin();it!=mat_ptr_full.end();)
        if((*it).users==0){
            mat_ptr_free.push((*it).mat);
            it=mat_ptr_full.erase(it);
        } else it++;
}


FQ::FQ(){}
void FQ::setUserFps(double nfps, unsigned maxframes){
    std::lock_guard<std::mutex>lock(umx);
    fps=nfps;
    maxfr=maxframes;
}
cv::Mat const* FQ::getUserMat(unsigned N){
    std::lock_guard<std::mutex>lock(umx);
    return (N>=full.size())?nullptr:(*full[N].ptr);
}
unsigned int FQ::getUserTimestamp(unsigned N){
    std::lock_guard<std::mutex>lock(umx);
    return (N>=full.size())?0:(full[N].timestamp);
}
void FQ::freeUserMat(unsigned N){
    std::lock_guard<std::mutex>lock(umx);
    if (N<full.size()){
        free.push_back(full[N].ptr);
        full.erase(full.begin()+N);
    }
}
unsigned FQ::getFullNumber(){
    std::lock_guard<std::mutex>lock(umx);
    return full.size();
}
unsigned FQ::getFreeNumber(){
    std::lock_guard<std::mutex>lock(umx);
    return free.size();
}
