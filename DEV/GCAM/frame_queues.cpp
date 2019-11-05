#include "DEV/GCAM/frame_queues.h"

FQsPC::FQsPC(){}
bool FQsPC::isThereInterest(){
    std::lock_guard<std::mutex>lock(qmx);
    std::list<FQ>::iterator it=user_queues.begin();
    for (int i=0;i!=user_queues.size();i++){
        if(it->umx!=nullptr){
            std::lock_guard<std::mutex>lock(*it->umx);
            if (it->fps!=0) return true;
        }
        it++;
    }
    return false;
}
cv::Mat* FQsPC::getAFreeMatPtr(){
    std::lock_guard<std::mutex>lock(qmx);
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
    std::lock_guard<std::mutex>lock(qmx);
    camfps=fps;
    mat_ptr_full.push_front({mat,0,timestamp});
    if ((mat)->rows==0){
        std::cerr<<"ERROR: FQ::enqueueMat says rows are 0. Will not enqueue, returning.\n";
        mat_ptr_full.pop_front();
        return;
    }
    std::list<FQ>::iterator it=user_queues.begin();
    for (int i=0;i!=user_queues.size();i++){
        if(it->umx!=nullptr)(it->umx->lock());
        if (it->fps!=0){
            if (it->maxfr==0 || it->full.size()<it->maxfr) it->div=round(camfps/it->fps);
            else {it->div=0; if(it->umx!=nullptr)it->umx->unlock(); std::advance(it,1); continue;}

            if (it->i>=it->div){
                it->i=1;
                it->full.push_back({&mat_ptr_full.front().mat,timestamp});
                mat_ptr_full.front().users++;
            }
            else it->i++;
        }
        if(it->umx!=nullptr)(it->umx->unlock());
        it++;
    }
    if(mat_ptr_full.front().users==0){
        mat_ptr_full.pop_front();
        mat_ptr_free.push(mat);
    }
    reclaim();
}
unsigned FQsPC::getFreeNumber(){
    std::lock_guard<std::mutex>lock(qmx);
    return mat_ptr_free.size();
}
unsigned FQsPC::getFullNumber(){
    std::lock_guard<std::mutex>lock(qmx);
    return mat_ptr_full.size();
}
FQ* FQsPC::getNewFQ(){
    FQ* ret;
    std::lock_guard<std::mutex>lock(qmx);
    user_queues.emplace_back();
    return &user_queues.back();
}
void FQsPC::deleteFQ(FQ* fq){
    if(fq==nullptr) return; //already deleted
    std::lock_guard<std::mutex>lock(qmx);
    fq->setUserFps(0);
    while(fq->getFullNumber()) fq->freeUserMat();
    delete fq->umx;
    fq->umx=nullptr;  //actual deleting done in FQsPC::reclaim()
    fq=nullptr;
}
void FQsPC::setCamFPS(double nfps){
    std::lock_guard<std::mutex>lock(qmx);
    fps=nfps;
}
void FQsPC::reclaim(){
    std::list<FQ>::iterator it=user_queues.begin();
    for (int i=0;i!=user_queues.size();i++){
        if(it->umx==nullptr) if(it->free.empty()){
            std::list<FQ>::iterator itt=it;
            it++;
            user_queues.erase(itt); i--;
            continue;
        }
        if(it->umx!=nullptr)(it->umx->lock());
        for (std::list<_used>::iterator itu=mat_ptr_full.begin();itu!=mat_ptr_full.end();itu++)
            for (int j=0;j!=it->free.size();j++)
                if(it->free[j]==(&(*itu).mat)){
                    (*itu).users--;
                    it->free.erase(it->free.begin()+j);
                    j--;
                }
        if(it->umx!=nullptr)(it->umx->unlock());
        it++;
    }
    for (std::list<_used>::iterator it=mat_ptr_full.begin();it!=mat_ptr_full.end();)
        if((*it).users==0){
            mat_ptr_free.push((*it).mat);
            it=mat_ptr_full.erase(it);
        } else it++;
}


FQ::FQ(){umx=new std::mutex;}
void FQ::setUserFps(double nfps, unsigned maxframes){
    std::lock_guard<std::mutex>lock(*umx);
    fps=nfps;
    maxfr=maxframes;
}
cv::Mat const* FQ::getUserMat(unsigned N){
    std::lock_guard<std::mutex>lock(*umx);
    return (N>=full.size())?nullptr:(*full[N].ptr);
}
unsigned int FQ::getUserTimestamp(unsigned N){
    std::lock_guard<std::mutex>lock(*umx);
    return (N>=full.size())?0:(full[N].timestamp);
}
void FQ::freeUserMat(unsigned N){
    std::lock_guard<std::mutex>lock(*umx);
    if (N<full.size()){
        free.push_back(full[N].ptr);
        full.erase(full.begin()+N);
    }
}
unsigned FQ::getFullNumber(){
    std::lock_guard<std::mutex>lock(*umx);
    return full.size();
}
unsigned FQ::getFreeNumber(){
    std::lock_guard<std::mutex>lock(*umx);
    return free.size();
}
