#include "frame_queues.h"

FQsPC::FQsPC() : fps(30){}
bool FQsPC::isThereInterest(){
    std::lock_guard<std::mutex>lock(userqmx);
    for (int i=0;i!=user_queues.size();i++)
        if (user_queues[i].fps!=0) return true;
    return false;
}
cv::Mat* FQsPC::getAFreeMatPtr(){
    if (mat_ptr_free.empty()){
        reclaim();
        if (mat_ptr_free.empty()){
            mat_reservoar.emplace_back();
            mat_ptr_free.push(&mat_reservoar.back());
        }
    }
    return mat_ptr_free.front();
}
void FQsPC::enqueueMat(unsigned int timestamp){
    unsigned camfps;
    {std::lock_guard<std::mutex>lock(userqmx);
        camfps=fps;}
    mat_ptr_full.emplace_front(_used{mat_ptr_free.front(),0,timestamp});
    for (int i=0;i!=user_queues.size();i++){
        std::lock_guard<std::mutex>lock(user_queues[i].umx);
        if (user_queues[i].fps!=0){
            if (user_queues[i].maxfr==0 || user_queues[i].full.size()==0) user_queues[i].div=round(camfps/user_queues[i].fps);
            else if (user_queues[i].full.size()<=user_queues[i].maxfr/2 && user_queues[i].div!=round(camfps/user_queues[i].fps)) user_queues[i].div-=1;
            else if (user_queues[i].full.size()>=user_queues[i].maxfr) user_queues[i].div+=1;
            //std::cerr<<"camfps/fps: "<<user_queues[i].div<<"\n";
            if (user_queues[i].i>=user_queues[i].div){
                user_queues[i].i=1;
                user_queues[i].full.push({&mat_ptr_full.front().mat,timestamp});
                mat_ptr_full.front().users++;
            }
            else user_queues[i].i++;
        }
    }
    if(mat_ptr_full.front().users==0) mat_ptr_full.pop_front();
    else mat_ptr_free.pop();
    reclaim();
}
unsigned FQsPC::getMatNumber(){
    return mat_reservoar.size();
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
void FQsPC::setCamFPS(double nfps){
    std::lock_guard<std::mutex>lock(userqmx);
    fps=nfps;
}
void FQsPC::reclaim(){
    for (int i=0;i!=user_queues.size();i++){
        std::lock_guard<std::mutex>lock(user_queues[i].umx);
        for (int k=0;k!=mat_ptr_full.size();k++)
            for (int j=0;j!=user_queues[i].free.size();j++)
                if(user_queues[i].free[j]==(&mat_ptr_full[k].mat)){
                    mat_ptr_full[k].users--;
                    user_queues[i].free.erase(user_queues[i].free.begin()+j);
                    j--;
                }
    }
    for (int k=0;k!=mat_ptr_full.size();k++)
        if(mat_ptr_full[k].users==0){
            mat_ptr_free.push(mat_ptr_full[k].mat);
            mat_ptr_full.erase(mat_ptr_full.begin()+k);
            k--;
        }
}


FQ::FQ() : fps(0), i(1){}
void FQ::setUserFps(double nfps, unsigned maxframes){
    std::lock_guard<std::mutex>lock(umx);
    fps=nfps;
    maxfr=maxframes;
}
cv::Mat const* FQ::getUserMat(){
    std::lock_guard<std::mutex>lock(umx);
    return (full.empty())?nullptr:(*full.front().ptr);
}
unsigned int FQ::getUserTimestamp(){
    std::lock_guard<std::mutex>lock(umx);
    return (full.empty())?0:(full.front().timestamp);
}
void FQ::freeUserMat(){
    std::lock_guard<std::mutex>lock(umx);
    if (!full.empty()){
        free.push_back(full.front().ptr);
        full.pop();
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
