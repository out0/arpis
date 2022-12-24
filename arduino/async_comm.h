#ifndef _ASYNC_COMMUNICATION_H
#define _ASYNC_COMMUNICATION_H

#include <stdint.h>

#include "protocol.h"

#define MAX_RCV_BUFFER_SIZE 64
#define MAX_SND_BUFFER_SIZE 64

class AsyncCommunication
{
private:
    char rcvBuffer[MAX_RCV_BUFFER_SIZE];
    char sndBuffer[MAX_SND_BUFFER_SIZE];
    uint8_t lastFrameId;
    uint8_t rcvBufferSize;
    uint8_t sndBufferSize;

    char serialRead()
    {
        waitBus();
        return busRead();
    }

protected:
    virtual void waitBus() = 0;
    virtual void busInitialize() = 0;
    virtual unsigned int busBufferAvailableRead() = 0;
    virtual char busRead() = 0;
    virtual void busWrite(char val) = 0;
    virtual unsigned int busBufferAvailableWrite() = 0;
    virtual void busFlush() = 0;
    virtual bool busReady() = 0;

public:
    AsyncCommunication()
    {
    }

    void initialize() 
    {
        busInitialize();
        rcvBufferSize = 0;
        sndBufferSize = 0;
    }

    char read(uint8_t pos) 
    {
        return rcvBuffer[pos];
    }

    uint16_t readInt16(uint8_t pos)
    {
        uint16p p;
        p.bval[0] = read(pos);
        p.bval[1] = read(pos+1);
        return p.val;
    }

    bool hasDataToSend() 
    {
        return sndBufferSize > 0;
    }

    void write(char val) 
    {
        sndBuffer[sndBufferSize++] = val;
    }

    void writeF(float val) 
    {
        unsigned char const *p = reinterpret_cast<unsigned char const *>(&val);
        for (int i = 0; i < sizeof(float); i++)
            sndBuffer[sndBufferSize++] = p[i];
    }

    void writeL(long val) 
    {
        unsigned char const *p = reinterpret_cast<unsigned char const *>(&val);
        for (int i = 0; i < sizeof(long); i++)
            sndBuffer[sndBufferSize++] = p[i];
    }

    void receiveData() 
    {
        if (rcvBufferSize > 0)
            return;

        if (busBufferAvailableRead() == 0)
            return;

        rcvBufferSize = 0;
        char ch;

        bool valid = false;
        while (!valid && busBufferAvailableRead() > 0)
        {
            if (serialRead() == MSG_START)
                valid = true;
        }

        if (!valid)
        {
            rcvBufferSize = 0;
            return;
        }

        valid = false;
        while (!valid && busBufferAvailableRead() > 0 && rcvBufferSize < MAX_RCV_BUFFER_SIZE)
        {
            ch = serialRead();
            if (ch == MSG_END)
                valid = true;
            else
                rcvBuffer[rcvBufferSize++] = ch;
        }

        if (!valid)
            rcvBufferSize = 0;

        lastFrameId = rcvBuffer[0];
    }

    void sendData(uint8_t frameId, uint8_t msgType) 
    {
        if (sndBufferSize == 0)
            return;

        if (busBufferAvailableWrite() < sndBufferSize)
            return;

        busWrite(MSG_START);
        busWrite(frameId);
        busWrite(msgType);

        for (int i = 0; i < sndBufferSize; i++)
            busWrite(sndBuffer[i]);

        busWrite(MSG_END);

        sndBufferSize = 0;

        busFlush();
    }

    bool hasData() 
    {
        return rcvBufferSize > 0;
    }

    void ack() 
    {
        write(MSG_ACK);
        sendData(lastFrameId, PROTOCOL_FRAME_TYPE_ACK);
    }

    void nack() 
    {
        write(MSG_ERR);
        sendData(lastFrameId, PROTOCOL_FRAME_TYPE_ACK);
    }

    bool isReady() 
    {
        return busReady();
    }

    void clear() 
    {
        while (busBufferAvailableRead() > 0)
            busRead();

        sndBufferSize = 0;
        rcvBufferSize = 0;
    }

    void clearReceiveBuffer() 
    {
        rcvBufferSize = 0;
    }
};

#endif
