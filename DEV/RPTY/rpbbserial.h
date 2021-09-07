#ifndef RPBBSERIAL_H
#define RPBBSERIAL_H

#include "fpga_const.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include "_config.h"

class RPBBSerial{       // Red Pitaya BitBang Serial
private:
    void __throw_serial_ack();
protected:
    bool inited_o=false;
    bool inited_i=false;
    uint8_t  _gpioN_o, _gpioP_o;
    uint8_t  _gpioN_i, _gpioP_i;
    double _clock;
    double baudrate;
    double stopbits;
    uint32_t _cyclesPerBit;
    uint32_t cyclesPerStopBit;

    rtoml::vsr conf;

public:
    const uint8_t& gpioN_o{_gpioN_o};
    const uint8_t& gpioP_o{_gpioP_o};
    const uint8_t& gpioN_i{_gpioN_i};
    const uint8_t& gpioP_i{_gpioP_i};
    const uint32_t& cyclesPerBit{_cyclesPerBit};
    const double& clock{_clock};
    RPBBSerial();
    void serial_init(std::vector<uint32_t> &commands, int8_t gpio_o, int8_t gpio_i=-1, double baudrate = 115200, double clock = 125000000, double stopbits=1);         // initialize serial
                                                                                        // gpio pin for serial, N (0-7), P (8-15), disabled (-1) (you can disable either input or output if not needed)
                                                                                        // you can exectue the commands immediately, this sets the gpio directions and values and initializes the class variables
    void serial_ack(std::vector<uint32_t> &commands, unsigned ackqueue=0, unsigned num=0, const char breakChar='\n');  // serial acquisition
                                                                                        // the queue acquires num bytes into ackqueue, if num=0 it acquires until it receives the breakChar (default '\n')
                                                                                        // you can also set num to 1 and set the queue to loop
                                                                                        // for num=0 and when looping num=1, you need to add a trigger condition to the command queue before calling serial_ack, make it loop and then trigger it to start looping
                                                                                        // its also a good idea to reset ackqueue before calling this
                                                                                        // note that, for num=0 this function will use FLAGS_LOCAL 0x8000 (last bit)
    void string_to_command(std::string string, std::vector<uint32_t> &commands, uint16_t W4FLAGS_SHARED=0x0000);        // generates the commands necessary to transmit the provided string
                                                                                        // if W4FLAGS_SHARED!=0 it will wait for W4FLAGS_SHARED to go high before sending the last character (which should be a newline), this allows for precise timing by triggering it from another queue
                                                                                        // with the last three arguments you can override settings
    void convert_data_to_string(uint32_t *data, uint32_t size4, std::string &string);   // note: size4 is number of uint32_t (4 bytes each), so bits, not bytes, also size4 has to be a multiple of 8 (ie. partial transmissions will throw an exception)
                                                                                        // this appends to string
    std::string convert_data_to_string(uint32_t *data, uint32_t size4);                 // for convenience
};
// NOTE: none of these functions actually send the commands, they just generate them. Make sure you actually send the init command before sending any others.
// NOTE: these commands append to the provided command queue

#endif // RPBBSERIAL_H
