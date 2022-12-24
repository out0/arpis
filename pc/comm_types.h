#ifndef _COMM_TYPES_H
#define _COMM_TYPES_H

#include <stdint.h>
#include <functional>

typedef unsigned char uchar;

typedef union
{
    float fval;
    char bval[4];
} floatp;

typedef union
{
    uint16_t val;
    char bval[2];
} uint16p;

typedef union
{
    long val;
    char bval[4];
} longp;

class ResponseData
{
public:
    char *data;
    unsigned int size;
    uchar frameId;
    uchar frameType;
    uchar deviceId;
};

class ISerialLink
{
public:
    virtual void addHandler(uchar deviceId, uchar handlerId, std::function<void(ResponseData *)> &func) = 0;
    virtual void removeHandler(uchar deviceId, uchar handlerId) = 0;
    virtual bool hasHandler(uchar deviceId, uchar handlerId) = 0;
    virtual bool syncRequest(uchar deviceId) = 0;
    virtual bool syncRequest(uchar deviceId, uchar val1) = 0;
    virtual bool syncRequest(int deviceId, uchar val1, uchar val2) = 0;
    virtual bool syncRequest(int deviceId, uchar val1, uint16_t val2) = 0;
    virtual bool syncRequest(int deviceId, uchar val1, uchar val2, uchar val3) = 0;
    virtual void asyncRequest(uchar deviceId) = 0;
    virtual void asyncRequest(uchar deviceId, uchar val1) = 0;
    virtual void asyncRequest(int deviceId, uchar val1, uchar val2) = 0;
    virtual void asyncRequest(int deviceId, uchar val1, uchar val2, uchar val3) = 0;
};

// template <typename T1, typename T2>
// class Tuple {
// public:
//     Tuple(T1 first, T2 second) {
//         this->first = first;
//         this->second = second;
//     }

//     T1 first;
//     T2 second;
// };

#endif
