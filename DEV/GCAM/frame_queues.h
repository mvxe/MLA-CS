#ifndef FRAME_QUEUES_H
#define FRAME_QUEUES_H

#include <string>
#include <deque>
#include <stack>
#include <list>
#include <mutex>
#include "opencv2/opencv.hpp"

class FQ;
class camobj;

class FQsPC      //this class should let threads exchange images safely (Frame Queues Per Camera)
{
        friend class camobj;
public:
    FQsPC();
    ~FQsPC();

    unsigned getFreeNumber();       //get the number of free matrices
    unsigned getFullNumber();       //get the number of full matrices

    FQ* getNewFQ();                 //returns a pointer to a new frame queue
    void deleteFQ(FQ* fq);
    double getActualFps(){std::lock_guard<std::mutex>lock(qmx); return actualFps;}

protected:
    cv::Mat* getAFreeMatPtr();
    void enqueueMat(cv::Mat* mat, unsigned int timestamp);
    double isThereInterest();       //this returns the highest requested FPS, 0 if no interest
    void setActualFps(double fps){std::lock_guard<std::mutex>lock(qmx); actualFps=fps;}

private:
    struct _used{
        cv::Mat* mat;
        unsigned users;              //number of users that got a copy of this pointer
        long unsigned timestamp;     //timestamp (this is the internal camera timestamp)
    };

    void reclaim();
    std::queue<cv::Mat*> mat_ptr_free;      //contains pointers to actual data that is free
    std::list<_used> mat_ptr_full;          //contains pointers to actual data that is waiting to be processed
    std::list<FQ> user_queues;             //contains of the user queues
    std::mutex qmx;

    double actualFps{0};
};

/*########## FQ ##########*/

class FQ      //frame queue,        this should be accessed from  the user thread only
{
    friend class FQsPC;
public:
    FQ();
    void setUserFps(double nfps, unsigned maxframes = 0);   //sets the framerate, set to zero if not in use to avoid piling up of data, see below, the optional parameter maxframes doesnt add new frames until the total number is smaller than maxframes
    cv::Mat const* getUserMat(unsigned N=0);                //gets the pointer to the N-th (oldest if no argument) matrix, if non existant, return nullptr (the larger the N, the newer the matrix)
    unsigned int getUserTimestamp(unsigned N=0);            //gets the timestamp of the N-th (oldest if no argument) matrix
    void freeUserMat(unsigned N=0);                         //tells the class its safe to free N-th (oldest if no argument) matrix (if this isnt called, the above call will return the pointer to the same old matrix)
    unsigned getFullNumber();                               //get the number of full matrices (return the number of frames waiting to be processed by this user)
    unsigned getFreeNumber();                               //get the number of free matrices
    struct _img{
        cv::Mat** ptr;
        long unsigned timestamp;     //timestamp (this is the internal camera timestamp)
    };
private:
    std::deque<_img> full;
    std::deque<cv::Mat**> free;
    double fps{0};                 //rounds to the closest divisor by two of the actual framerate (ie 9999 of 168 fps gives 168, 33 of 168 fps gives 28 fps, 0 means no acquisition)
    unsigned maxfr;
    unsigned div;
    int i{1};                      //for framerate reduction
    std::mutex* umx;
};

#endif // FRAME_QUEUES_H
