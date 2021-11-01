#ifndef CAMOBJ_H
#define CAMOBJ_H

#include <arv.h>
#include "DEV/GCAM/arvwrap.h"
#include "DEV/GCAM/frame_queues.h"
#include <vector>
#include "_config.h"

const int FRAMEBUFFER_INITIAL_SIZE = 10;   //starting size of framebuffer
class GCAM;

class camobj: public camobj_config{     // note: currently this tagets features implemented by Basler, as the default setFPS from aravis didnt work. This means genicams other than Basler may not work properly.
    friend class GCAM;
    friend class gcam_config;
    friend class FrameObserver;
public:
    void set(std::string atr, bool nvar);
    void set(std::string atr, long int nvar);
    void set(std::string atr, double nvar);
    void set(std::string atr, std::string nvar);
    bool        get_bool(std::string atr);
    long int    get_lint(std::string atr);
    double      get_dbl(std::string atr);
    std::string get_str(std::string atr);
    template <typename T> T get(std::string atr);
    void run(std::string atr);

    void set_trigger(std::string trig="none");
    void get_frame_rate_bounds (double *min, double *max);

    std::mutex mkmx;
    tsvar<std::string> selected_ID;                         //thread safe access to select camera ID
    const std::atomic<bool>& connected{_connected};         //thread safe access to camera status
    std::atomic<bool> checkID{true};

    FQsPC FQsPCcam;                                         //use this to get frame queue from camera, see frame_queues.h for commands

    typedef struct {
        GMainLoop *main_loop;
        camobj *cam;
        int buffer_count;
    } ApplicationData;

    const int& camCols{Xsize};
    const int& camRows{Ysize};
private:
    std::atomic<bool> _connected{false};
    camobj(std::string strID);
    static void control_lost_cb (ArvDevice *ldev);
    static void new_frame_ready (ArvStream *stream, camobj* camm);
    std::atomic<bool> control_lost{false};

    void start();
    void work();                                //call this periodically
    void end();                                 //mutexed
    void con_cam();
    // implementing a ROI function doesnt work with camera(Basler) and api(Aravis), so I give up on that

    void pushFrameIntoStream();
    void requeueFrame(cv::Mat* MatPtr);

    void resetTimeout();

    static GCAM *cobj;
    int lost_frames_GCAM_VMB{0};

    ArvCamera* cam{NULL};
    ArvDevice* dev{NULL};
    ArvStream* str{NULL};
    std::string ID{"none"};     //actual camera ID

    GMainContext *context;
    GMainLoop *main_loop;   //glib main loop, needed for frame buffer ready callbacks

    std::queue<ArvBuffer*> FreeBuffers;
    std::deque<ArvBuffer*> FullBuffers;
    gint payload;
    int Xsize;
    int Ysize;
    double ackFPS{0};
    double actualAckFPS{0};
    std::string format;

    std::mutex mtx;
    bool trigMode{false};
    std::chrono::time_point<std::chrono::system_clock> lastFrameTime;   // for timeout detection
    std::mutex lastFrameTimeMux;
};

#endif // CAMOBJ_H
