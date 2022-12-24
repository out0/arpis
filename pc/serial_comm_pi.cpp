#include "serial_comm_pi.h"
#include "../lib/include/crawler_hal.h"

char *SerialCommunication::buildSendMessage()
{
    char *msg = (char *)malloc(sizeof(unsigned char) * (sndBufferSize + 3));

    msg[0] = MSG_START;
    uint8_t i = 0;
    for (i = 0; i < sndBufferSize; i++)
    {
        msg[i + 1] = sndBuffer[i];
    }
    msg[i + 1] = MSG_END;
    msg[i + 2] = 0;
    return msg;
}

SerialCommunication::SerialCommunication(const char *device)
{
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "unable to initialize wiringPi\n");
        exit(1);
    }

    connFd = serialOpen(device, SERIAL_BOUND_RATE);
    if (connFd == -1)
    {
        fprintf(stderr, "unable to open device %s: %s\n", device, strerror(errno));
        exit(1);
    }

    sndBufferSize = 0;
    rcvBufferSize = 0;
}

int SerialCommunication::readByte()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SERIAL_WAIT_DELAY_ms));
    return serialGetchar(connFd);
}

void SerialCommunication::clearReceiveBuffer()
{

    while (serialDataAvail(connFd) > 0)
        serialGetchar(connFd);
    rcvBufferSize = 0;
}

bool SerialCommunication::receiveData() 
{
    if (rcvBufferSize > 0)
        return false;

    if (serialDataAvail(connFd) <= 0)
        return false;

    rcvBufferSize = 0;
    int ch;
    int resp = RCV_RESP_INVALID;

    // printf ("** buffer size: %d\n", serialDataAvail(connFd));

    bool valid = false;

    while (!valid && serialDataAvail(connFd) > 0)
    {
        ch = readByte();
        if (ch == MSG_START)
            valid = true;
#ifdef DEBUG
        else
            printf("ignoring: %d\n", ch);
#endif
    }

    if (!valid)
        return false;

    valid = false;

    while (!valid && serialDataAvail(connFd) > 0 && rcvBufferSize < RCV_BUFFER_SIZE)
    {
        ch = readByte();
        if (ch == MSG_END)
            return true;

        rcvBuffer[rcvBufferSize++] = (unsigned char)ch;
    }

#ifdef DEBUGf
    printf("RCV_RESP_INVALID [");
    for (int i = 0; i < rcvBufferSize; i++)
        printf(" %d", rcvBuffer[i]);
    printf(" ]\n");
#endif

    rcvBufferSize = 0;
    return false;
}

void SerialCommunication::sendData() 
{
    if (sndBufferSize == 0)
        return;

    char *msg = buildSendMessage();
#ifdef DEBUG
    printf("sending: [");
    for (int i = 0; i < sndBufferSize + 3; i++)
    {
        printf(" %d", msg[i]);
    }
    printf("]\n");
#endif
    serialPuts(connFd, msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERIAL_WAIT_DELAY_ms));
    sndBufferSize = 0;
}

bool SerialCommunication::hasData() 
{
    return rcvBufferSize > 0;
}

char SerialCommunication::read(unsigned int pos) 
{
    return rcvBuffer[pos];
}

float SerialCommunication::readF(unsigned int pos) 
{
    floatp p;
    for (uint8_t i = 0; i < 4; i++)
        p.bval[i] = rcvBuffer[pos++];
    return p.fval;
}

uint16_t SerialCommunication::readInt16(unsigned int pos) 
{
    uint16p p;
    p.bval[0] = rcvBuffer[pos++];
    p.bval[1] = rcvBuffer[pos++];
    return p.val;
}

void SerialCommunication::writeInt16(uint16_t val) 
{
    uint16p p;
    p.val = val;
    write(p.bval[0]);
    write(p.bval[1]);
}
void SerialCommunication::write(unsigned char val) 
{
    sndBuffer[sndBufferSize++] = val;
}

char *SerialCommunication::copy() 
{
    char *p = (char *)malloc(sizeof(char) * (rcvBufferSize + 1));
    memcpy(p, &rcvBuffer, rcvBufferSize);
    p[rcvBufferSize] = 0;
    return p;
}

unsigned int SerialCommunication::receivedDataSize() 
{
    return rcvBufferSize;
}

unsigned int SerialCommunication::sendDataSize() 
{
    return sndBufferSize;
}

void SerialCommunication::clearRcv() 
{
    // clearReceiveBuffer();
    rcvBufferSize = 0;
}

void SerialCommunication::clearSnd() 
{
    sndBufferSize = 0;
}
