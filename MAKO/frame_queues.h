#ifndef FRAME_QUEUES_H
#define FRAME_QUEUES_H

#include "includes.h"

class FQ;

class FQsPC      //this class should let threads exchange images safely (Frame Queues Per Camera)
{
public:
    FQsPC();

    bool isThereInterest();         //if this returns false dont bother filling a new matrix, nobody is interested in frames
    unsigned getMatNumber();        //get total number of allocated matrices
    unsigned getFullNumber();       //get the number of full matrices

    cv::Mat* getAFreeMatPtr();      //these two functions are used by the vimba api observer to put new frames into the queue
    void enqueueMat();              //they are not thread safe, so use them only in one observer function

    FQ* getNewFQ();                 //returns a pointer to a new frame queue
    void setCamFPS(double nfps);  //set the camera framerate, essential of proper image sorting into queues (this is thread safe)
private:
    struct _used{
        cv::Mat* mat;
        unsigned users;              //number of users that got a copy of this pointer
    };

    void reclaim();
    std::deque<cv::Mat> mat_reservoar;      //contains the actual image data
    std::queue<cv::Mat*> mat_ptr_free;      //contains pointers to actual data that is free
    std::deque<_used> mat_ptr_full;         //contains pointers to actual data that is waiting to be processed
    std::deque<FQ> user_queues;             //contains of the user queues
    std::mutex userqmx;
    double fps;
};

class FQ      //frame queue,        this should be accessed from  the user thread only
{
    friend class FQsPC;
public:
    FQ();
    void setUserFps(double nfps, unsigned maxframes = 0);   //sets the framerate, set to zero if not in use to avoid piling up of data, see below, the optional parameter maxframes halves the framerate when the full queue size reaches maxframes, and ups it again at maxframes/2 up to fps
    cv::Mat const* getUserMat();        //gets the pointer to the last matrix
    void freeUserMat();                 //tells the class its safe to free the matrix (if this isnt called, the above call will return the pointer to the same old matrix)
    unsigned getUserQueueLength();      //return the number of frames waiting to be processed by this user
    unsigned getFullNumber();           //get the number of full matrices
    unsigned getFreeNumber();           //get the number of free matrices
private:
    std::queue<cv::Mat**> full;
    std::deque<cv::Mat**> free;
    double fps;                 //rounds to the closest divisor by two of the actual framerate (ie 9999 of 168 fps gives 168, 33 of 168 fps gives 28 fps, 0 means no acquisition)
    unsigned maxfr;
    unsigned div;
    int i;                      //for framerate reduction
    std::mutex umx;
};

#endif // FRAME_QUEUES_H
