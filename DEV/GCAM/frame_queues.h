#ifndef FRAME_QUEUES_H
#define FRAME_QUEUES_H

#include <string>
#include <deque>
#include <stack>
#include <list>
#include <mutex>
#include "opencv2/opencv.hpp"

class FQ;

class FQsPC      //this class should let threads exchange images safely (Frame Queues Per Camera)
{
public:
    FQsPC();

    bool isThereInterest();         //if this returns false dont bother filling a new matrix, nobody is interested in frames
    unsigned getFreeNumber();       //get the number of free matrices
    unsigned getFullNumber();       //get the number of full matrices

    FQ* getNewFQ();                 //returns a pointer to a new frame queue
    void setCamFPS(double nfps);    //set the camera framerate, essential of proper image sorting into queues (this is thread safe)
    void deleteFQ(FQ* fq);

    cv::Mat* getAFreeMatPtr();      //these two functions are used by the vimba api observer to put new frames into the queue           //TODO these two functions should not be exposed to the user, just FrameObserver::FrameReceived
    void enqueueMat(cv::Mat* mat, unsigned int timestamp);

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

    double fps{30};
};

/*########## FQ ##########*/

class FQ      //frame queue,        this should be accessed from  the user thread only
{
    friend class FQsPC;
public:
    FQ();
    void setUserFps(double nfps, unsigned maxframes = 0);   //sets the framerate, set to zero if not in use to avoid piling up of data, see below, the optional parameter maxframes doesnt add new frames untill the total number is smaller than maxframes
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
