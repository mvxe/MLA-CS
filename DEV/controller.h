#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <UTIL/.rtoml/rtoml.hpp>
#include <map>

// provides a standard interface to common functionality: motion, digital outputs etc
// functions will throw an exception if an error is encountered, for example calling functions on devices of wrong type
// public function of CTRL should be thread safe

class CTRL{

public:
    virtual ~CTRL(){};
    // configuration file; load is called on registerDevice() for that device to retrieve configuration
    // CTRL does not autosave, you need to call save before destroying CTRL
    rtoml::vsr conf;

            // #### device initialization ####

    // device types:
    enum devType{dt_motion, dt_gpio, dt_timer};

    // creates device objects and adds config entries
    virtual void registerDevice(std::string ID, devType type)=0;

    // inits all non-inited devices; some or all config options get frozen once inited
    virtual void initDevices()=0;

    // deinits all inited devices; configs get editable again
    virtual void deinitDevices()=0;

    // motion specific commands - affects all inited devices of type dt_motion
    // finds reference for all inited axes
    virtual void referenceMotionDevices()=0;

    // for convenience; returns a list of registered (motion)devices
    virtual std::vector<std::string> getMotionDevices()=0;
    virtual std::vector<std::string> getDevices()=0;

            // #### higher level functions: ####

    // motion settings:
    enum mst{mst_position, mst_minPosition, mst_maxPosition, mst_lastPosition, mst_restPosition, mst_defaultVelocity, mst_maximumVelocity, mst_defaultAcceleration, mst_maximumAcceleration, mst_mininumStep};

    virtual double getMotionSetting(std::string ID, mst setting)=0;
    // ID has to be a device of type dt_motion

    // motion flags:
    typedef uint16_t motionFlags;
    enum _motionFlags : motionFlags{
        MF_RELATIVE=1<<0,       // move position is relative instead of absolute
    };

    virtual void motion(std::string ID, double position, double velocity=0, double acceleration=0, motionFlags flags=0)=0;
    // ID has to be a device of type dt_motion
    // position gets rounded within mst_mininumStep

    virtual int getError(std::string ID)=0;
    // returns error code, 0 is no error; other codes very much depend on the type of device

    virtual void setGPIO(std::string ID, bool state)=0;
    // ID has to be a device of type dt_gpio and configured as output

    virtual void pulseGPIO(std::string ID, double duration)=0;
    // ID has to be a device of type dt_gpio and configured as output, sets GPIO high, and after duration it sets it to low (blocking)

            // #### convenience functions: ####

    // after referencing, it might be convenient to move stages into last postition using
    void motionDevicesToLastPosition(){
        for(auto& md : getMotionDevices())
            motion(md, getMotionSetting(md,CTRL::mst_lastPosition), getMotionSetting(md,CTRL::mst_maximumVelocity), getMotionSetting(md,CTRL::mst_maximumAcceleration));
    }

    // before deiniting, it might be convenient to move the stages to the rest position (the specific implementation should not update lastPosition after this move) using
    void motionDevicesToRestPosition(){
        for(auto& md : getMotionDevices())
            motion(md, getMotionSetting(md,CTRL::mst_restPosition), getMotionSetting(md,CTRL::mst_maximumVelocity), getMotionSetting(md,CTRL::mst_maximumAcceleration));
    }


public:
    //functions that are accessed by the command object (CO)
    class CO;

    // block execution until condition is met
    enum _holdCondition{he_gpio_low, he_gpio_high,      // for dt_gpio
                    he_timer_done,                  // for dt_timer
                    he_motion_ontarget              // for dt_motion
                   };
protected:

    // construct command object
    virtual void CO_init(CO* a)=0;

    // delete command object
    virtual void CO_delete(CO* a)=0;

    // execute command object
    virtual void CO_execute(CO* a)=0;
    // NOTE: the object is not cleared on execution

    // add singe axis move to command object (for multi axis movements just call for each axis)
    virtual void CO_addMotion(CO* a, std::string ID, double position, double velocity=0, double acceleration=0, motionFlags flags=0)=0;
    // ID has to be a device of type dt_motion
    // the object should handle rounding errors, the final resulting position gets rounded within mst_mininumStep

    // add delay to command object : blocks the execution for a certain ammount of time
    virtual void CO_addDelay(CO* a, double delay)=0;

    virtual void CO_setGPIO(CO* a, std::string ID, bool state)=0;
    // ID has to be a device of type dt_gpio and configured as output

    virtual void CO_pulseGPIO(CO* a, std::string ID, double duration)=0;
    // ID has to be a device of type dt_gpio and configured as output, sets GPIO high, and after duration it sets it to low (blocking)

    virtual void CO_addHold(CO* a, std::string ID, _holdCondition condition)=0;
    // ID has to be of the right type (see _holdEvent)

    virtual void CO_startTimer(CO* a, std::string ID, double duration)=0;
    // ID has to be a device of type dt_timer, a hold condition of "$ID_DONE" will bock until the timer expires

    // clear command object data
    virtual void CO_clear(CO* a)=0;

public:

    class CO{       // command object - defines a specific procedure (such as a motion 'trajectory' combined with gpio triggering) attached to a specific controller
    public:         // for more info on each function, see comments above (CO_function)  
        CO(CTRL* ctrl):ctrl(ctrl){ctrl->CO_init(this);}
        ~CO(){ctrl->CO_delete(this);}
        void execute()
        {ctrl->CO_execute(this);}
        void addMotion(std::string ID, double position, double velocity=0, double acceleration=0, CTRL::motionFlags flags=0)
        {ctrl->CO_addMotion(this, ID, position, velocity, acceleration, flags);}
        void addDelay(double delay)
        {ctrl->CO_addDelay(this, delay);}
        void setGPIO(std::string ID, bool state)
        {ctrl->CO_setGPIO(this, ID, state);}
        void pulseGPIO(std::string ID, double duration)
        {ctrl->CO_pulseGPIO(this, ID, duration);}
        void addHold(std::string ID, _holdCondition condition)
        {ctrl->CO_addHold(this, ID, condition);}
        void startTimer(std::string GPIOID, double duration)
        {ctrl->CO_startTimer(this, GPIOID, duration);}
        void clear()
        {ctrl->CO_clear(this);}
        CTRL* const& _ctrl{ctrl};
    private:
        CTRL* ctrl;
    };
    friend CTRL::CO;

};

#endif // CONTROLLER_H
