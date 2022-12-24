#ifndef _USB_SERIAL_COMMUNICATION_H
#define _USB_SERIAL_COMMUNICATION_H

#include <Arduino.h>
#include <stdint.h>

#include "async_comm.h"
#include "protocol.h"

#include <SoftwareSerial.h>

#define SERIAL_BOUND_RATE 115200
#define SERIAL_RCV_WAIT_DELAY_ms 2
#define RCV_BUFFER_SIZE 64
#define SND_BUFFER_SIZE 64

class SoftwareSerialCommunication : public AsyncCommunication
{
private:
    SoftwareSerial *ss;
    int rxPin;
    int txPin;

protected:
    void waitBus() override
    {
        delay(SERIAL_RCV_WAIT_DELAY_ms);
    }
    void busInitialize() override
    {
        ss = new SoftwareSerial(rx, tx);
        ss->listen();
    }
    unsigned int busBufferAvailableRead() override
    {
        return ss->available();
    }
    char busRead() override
    {
        return ss->read();
    }
    void busWrite(char val) override
    {
        ss->write(val);
    }
    unsigned int busBufferAvailableWrite() override
    {
        return ss->availableForWrite();
    }
    void busFlush() override
    {
        ss->flush();
    }
    bool busReady() override
    {
        return ss != nullptr;
    }

public:
    SoftwareSerialCommunication(int rxPin, int txPin)
    {
        this->rxPin = rxPin;
        this->txPin = txPin;
    }
};

#endif
