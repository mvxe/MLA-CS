#include "frame_queues.h"


FQsPC::FQsPC() : fps(30){}
bool FQsPC::isThereInterest(){
    bool ret=false;
    userqmx.lock();
    for (int i=0;i!=user_queues.size();i++)
        if (user_queues[i].fps!=0) {ret=true;break;}
    userqmx.unlock();
    return ret;
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
void FQsPC::enqueueMat(){
    userqmx.lock();
    unsigned camfps=fps;
    userqmx.unlock();
    mat_ptr_full.emplace_front(_used{mat_ptr_free.front(),0});
    for (int i=0;i!=user_queues.size();i++){
        user_queues[i].umx.lock();
            if (user_queues[i].fps!=0){
                if (user_queues[i].maxfr==0 || user_queues[i].full.size()==0) user_queues[i].div=round(camfps/user_queues[i].fps);
                else if (user_queues[i].full.size()<=user_queues[i].maxfr/2 && user_queues[i].div!=round(camfps/user_queues[i].fps)) user_queues[i].div-=1;
                else if (user_queues[i].full.size()>=user_queues[i].maxfr) user_queues[i].div+=1;
                //std::cerr<<"camfps/fps: "<<user_queues[i].div<<"\n";
                if (user_queues[i].i>=user_queues[i].div){
                    user_queues[i].i=1;
                    user_queues[i].full.push(&mat_ptr_full.front().mat);
                    mat_ptr_full.front().users++;
                }
                else user_queues[i].i++;
            }
        user_queues[i].umx.unlock();
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
    userqmx.lock();
    user_queues.emplace_back();
    ret=&user_queues.back();
    userqmx.unlock();
    return ret;
}
void FQsPC::setCamFPS(double nfps){
    userqmx.lock();
    fps=nfps;
    userqmx.unlock();
}
void FQsPC::reclaim(){
    for (int i=0;i!=user_queues.size();i++){
        user_queues[i].umx.lock();
        for (int k=0;k!=mat_ptr_full.size();k++)
            for (int j=0;j!=user_queues[i].free.size();j++)
                if(user_queues[i].free[j]==(&mat_ptr_full[k].mat)){
                    mat_ptr_full[k].users--;
                    user_queues[i].free.erase(user_queues[i].free.begin()+j);
                    j--;
                }
        user_queues[i].umx.unlock();
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
    umx.lock();
    fps=nfps;
    maxfr=maxframes;
    umx.unlock();
}
cv::Mat const* FQ::getUserMat(){
    cv::Mat* ret;
    umx.lock();
    if (!full.empty()) ret=*full.front();
    else ret=nullptr;
    umx.unlock();
    return ret;
}
void FQ::freeUserMat(){
    umx.lock();
    if (!full.empty()){
        free.push_back(full.front());
        full.pop();
    }
    umx.unlock();
}
unsigned FQ::getFullNumber(){
    unsigned ret;
    umx.lock();
    ret=full.size();
    umx.unlock();
    return ret;
}
unsigned FQ::getFreeNumber(){
    unsigned ret;
    umx.lock();
    ret=free.size();
    umx.unlock();
    return ret;
}
