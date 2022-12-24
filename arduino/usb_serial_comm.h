#ifndef _USB_SERIAL_COMMUNICATION_H
#define _USB_SERIAL_COMMUNICATION_H

#include <Arduino.h>
#include <stdint.h>

#include "async_comm.h"
#include "protocol.h"

#define SERIAL_BOUND_RATE 115200
#define SERIAL_RCV_WAIT_DELAY_ms 2
#define RCV_BUFFER_SIZE 64
#define SND_BUFFER_SIZE 64

class UsbSerialCommunication : public AsyncCommunication
{
protected:
    void waitBus() override
    {
        delay(SERIAL_RCV_WAIT_DELAY_ms);
    }
    void busInitialize() override
    {
        Serial.begin(SERIAL_BOUND_RATE);
    }
    unsigned int busBufferAvailableRead() override
    {
        return Serial.available();
    }
    char busRead() override
    {
        return Serial.read();
    }
    void busWrite(char val) override
    {
        Serial.write(val);
    }
    unsigned int busBufferAvailableWrite() override
    {
        return Serial.availableForWrite();
    }
    void busFlush() override
    {
        Serial.flush();
    }
    bool busReady() override
    {
        return (Serial);
    }
};

#endif
