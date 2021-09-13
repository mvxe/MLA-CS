#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <UTIL/.rtoml/rtoml.hpp>

// this provides a general purpose controller interface
// providing a standard interface to common functionality: motion, digital outputs etc 

class CTRL{

public:
    class CO;
    friend CTRL::CO;
    enum mst{mst_position, mst_minPosition, mst_maxPosition, mst_lastPosition, mst_restPosition, mst_defaultVelocity, mst_maximumVelocity, mst_defaultAcceleration, mst_maximumAcceleration};
    typedef uint16_t motionFlags;
    enum _motionFlags : motionFlags{
        MF_RELATIVE=1<<0,       // move position is relative instead of absolute
    };

private:
    //functions that are accessed by the command object (CO)
        // construct command object
    virtual void CO_init(CO* a)=0;
        // delete command object
    virtual void CO_delete(CO* a)=0;
        // execute command object
    virtual void CO_execute(CO* a)=0;
        // add singe axis move to command object (for multi axis movements just call for each axis)
    virtual void CO_addMotion(CO* a, std::string axisID, double position, double velocity=0, double acceleration=0, motionFlags flags=0)=0;
        // add delay to command object : blocks the execution for a certain ammount of time
    virtual void CO_addDelay(CO* a, double delay)=0;
        // add hold to command object : blocks the execution until the specified condition is realized
        // a number of standard hold conditions should be defined, such as "axisID_MOTION_ONTARGET"
    virtual void CO_addHold(CO* a, std::string condition)=0;
        // add GPIO event to command object : performs an event, eg. turn on laser
    virtual void CO_addGPIOEvent(CO* a, std::string GPIOID, bool state)=0;
        // clear command object data
    virtual void CO_clear(CO* a)=0;
public:
    virtual ~CTRL(){};

    rtoml::vsr conf;

        // ## higher level functions: ##
    virtual void motion(std::string axisID, double position, double velocity=0, double acceleration=0, motionFlags flags=0)=0;

    virtual double getMotionSetting(std::string axisID, mst setting)=0;
    virtual int getMotionError(std::string axisID)=0;       // returns error code, 0 is no error; this very much depends on the type of stage


        // ## aditional higher level functions: ##

    // OPERATING PROCEDURE:
    //      1. add devices
    //      2. change settings via either GUI or TOML
    //      3. init devices
    //      4. use
    //      5. uninit devices

        // ## device initialization ##          // just name them, the devices themselves are configured via the GUI/TOML configuration
    virtual void addMotionDevice(std::string axisID)=0;
    virtual void setMotionDeviceType(std::string axisID, std::string type)=0;   // useful for GUI config
    virtual void addGPIOEvent(std::string GPIOID)=0;

    virtual void initMotionDevices()=0;                 // inits all non-inited devices
    virtual void referenceMotionDevices()=0;
    virtual void retraceMotionDevices()=0;              // moves all motion devices to the last saved position (RPTY::motion overrides last saved positions)
    virtual void deinitMotionDevices()=0;               // inits all non-inited devices


    class CO{       // command object - defines a specific procedure (such as a motion trajectory combined with gpio triggering) attached to a specific controller
    public:
        CO(CTRL* ctrl):ctrl(ctrl){}
        ~CO(){ctrl->CO_delete(this);}
        void execute()                                                                                                      // executes the command object; NOTE: the object is not cleared on execution
            {ctrl->CO_execute(this);}
        void addMotion(std::string axisID, double position, double velocity=0, double acceleration=0, CTRL::motionFlags flags=0)  // adds a single-axis move
            {ctrl->CO_addMotion(this, axisID, position, velocity, acceleration, flags);}
        void addDelay(double delay)                                                                                         // blocks the execution for a certain ammount of time
            {ctrl->CO_addDelay(this, delay);}
        void addHold(std::string condition)                                                                                 // blocks the execution until the specified condition has taken place
            {ctrl->CO_addHold(this, condition);}
        void addGPIOEvent(std::string GPIOID, bool state)                                                                   // performs an GPIO event, eg. turn on laser
            {ctrl->CO_addGPIOEvent(this, GPIOID, state);}
        void clear()
            {ctrl->CO_clear(this);}
    private:
        CTRL* ctrl;
    };
};


#endif // CONTROLLER_H
