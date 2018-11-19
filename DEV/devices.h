#ifndef DEVICES_H
#define DEVICES_H

#include <string>
#include <deque>

class _device_temp
{
public:
    _device_temp(std::string type_name, int max_num_of_devices=0);
    const int max_num_of_devices;
    const std::string type_name;

    const std::deque<std::string>& getFlags() const{return flags;}
protected:
    std::deque<std::string> flags;
};

#endif // DEVICES_H
