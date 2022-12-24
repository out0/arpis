#ifndef _SERIAL_COMMUNICATION_PI
#define _SERIAL_COMMUNICATION_PI

#define SERIAL_BOUND_RATE 115200
#define SERIAL_WAIT_DELAY_ms 2
#define RCV_BUFFER_SIZE 100
#define SND_BUFFER_SIZE 100

#define MSG_START 32
#define MSG_END 31

#define RCV_RESP_NO_DATA 0
#define RCV_RESP_VALID 1
#define RCV_RESP_INVALID 2

#define PROTOCOL_FRAME_TYPE_DATA 1
#define PROTOCOL_FRAME_TYPE_ACK 2
#define PROTOCOL_FRAME_TYPE_DATA_LIST 3

#define PROTOCOL_ACK 1
#define PROTOCOL_NACK 2

//#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <pthread.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <sys/ioctl.h>

class ISerialCommunication
{
public:
    virtual bool receiveData() = 0;
    virtual void sendData() = 0;
    virtual bool hasData() = 0;
    virtual int readByte() = 0;
    virtual char read(unsigned int pos) = 0;
    virtual float readF(unsigned int pos) = 0;
    virtual uint16_t readInt16(unsigned int pos) = 0;
    virtual void writeInt16(uint16_t val) = 0;
    virtual void write(unsigned char val) = 0;
    virtual char *copy() = 0;
    virtual unsigned int receivedDataSize() = 0;
    virtual unsigned int sendDataSize() = 0;
    virtual void clearReceiveBuffer() = 0;
    virtual void clearRcv() = 0;
    virtual void clearSnd() = 0;
};

class SerialCommunication : public ISerialCommunication
{
private:
    int connFd;
    unsigned char rcvBuffer[RCV_BUFFER_SIZE];
    unsigned char sndBuffer[SND_BUFFER_SIZE];
    unsigned int rcvBufferSize;
    unsigned int sndBufferSize;

    char *buildSendMessage();

public:
    SerialCommunication(const char *device);

    int readByte() override;
    void clearReceiveBuffer() override;
    bool receiveData() override;
    void sendData() override;
    bool hasData() override;
    char read(unsigned int pos) override;
    float readF(unsigned int pos) override;
    uint16_t readInt16(unsigned int pos) override;
    void writeInt16(uint16_t val) override;
    void write(unsigned char val) override;
    char *copy() override;
    unsigned int receivedDataSize() override;
    unsigned int sendDataSize() override;
    void clearRcv() override;
    void clearSnd() override;
};

#endif