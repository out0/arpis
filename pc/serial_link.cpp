#include "serial_comm_pi.h"
#include "serial_link.h"

#include <unistd.h>

void SerialLink::initialize()
{
    run = true;
    this->rcvThread = new std::thread(&SerialLink::rcvThreadHandler, this);
    this->handlers = new std::map<uchar, std::vector<SerialLinkResponseCallback *> *>();
    comm->clearRcv();
    comm->clearSnd();
}

void SerialLink::lock()
{
    this->commMtx.lock();
}

void SerialLink::unlock()
{
    this->commMtx.unlock();
}

int SerialLink::wait()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SERIAL_WAIT_DELAY_ms));
    return SERIAL_WAIT_DELAY_ms;
}

void SerialLink::rcvThreadHandlerValid()
{
    ResponseData *rcvMsg = new ResponseData();
    rcvMsg->data = comm->copy();
    rcvMsg->size = comm->receivedDataSize();
    rcvMsg->frameId = rcvMsg->data[0];
    rcvMsg->frameType = rcvMsg->data[1];
    rcvMsg->deviceId = rcvMsg->data[2];

#ifdef DEBUG
    printf("received valid message: frameId: %d, frameType: %d, deviceId: %d, size: %d\n", rcvMsg->frameId, rcvMsg->frameType, rcvMsg->deviceId, rcvMsg->size);
#endif
    processData(rcvMsg);
}

void SerialLink::rcvThreadHandler()
{
    while (run)
    {
        if (comm->receiveData())
        {
            rcvThreadHandlerValid();
            comm->clearRcv();
        }
        else
            wait();
    }
}

void SerialLink::executeCallbackForMessageData(ResponseData *rcvMsg)
{
    auto it = this->handlers->find(rcvMsg->deviceId);

    if (it != this->handlers->end())
    {
#ifdef DEBUG
        printf("found handler for deviceId = %d\n", it->first);
        if (it->second == nullptr)
        {
            printf("handler for deviceId = %d is NULL!!\n", it->first);
        }
#endif
        if (it->second != nullptr)
        {
            auto vector = it->second;
            for (auto ptr = vector->begin(); ptr < vector->end(); ptr++)
            {
                (*ptr)->callback(rcvMsg);
            }
        }
        // it->second(rcvMsg);
    }
}

void SerialLink::printRawData(const char *sensorName, ResponseData *p)
{
    if (p == nullptr || p->data == nullptr)
    {
        printf("%s received null data!!\n", sensorName);
        return;
    }

    printf("%s received data. Size: %d [", sensorName, p->size);
    for (int i = 0; i < p->size; i++)
        printf(" %d", p->data[i]);
    printf(" ]\n");
}

void SerialLink::processListData(ResponseData *rcvMsg)
{
    int i = 2;
    while (i < rcvMsg->size)
    {
        ResponseData *subMsg = new ResponseData();
        subMsg->deviceId = rcvMsg->data[i];
        subMsg->frameId = 1;
        subMsg->frameType = PROTOCOL_FRAME_TYPE_DATA;
        subMsg->size = rcvMsg->data[++i];
        subMsg->data = (char *)malloc(sizeof(char) * (subMsg->size + 1));

        for (int j = 0; j < subMsg->size && i < rcvMsg->size; j++, i++)
            subMsg->data[j] = rcvMsg->data[i];

        processData(subMsg);
    }
}

void SerialLink::processData(ResponseData *rcvMsg)
{
    switch (rcvMsg->frameType)
    {
    case PROTOCOL_FRAME_TYPE_ACK:
        requestAckWaitCheck.checkAck(rcvMsg);
#ifdef DEBUG
        printf("data is ack\n");
#endif
        executeCallbackForMessageData(rcvMsg);
        break;
    case PROTOCOL_FRAME_TYPE_DATA_LIST:
        processListData(rcvMsg);
        break;
    case PROTOCOL_FRAME_TYPE_DATA:
        executeCallbackForMessageData(rcvMsg);
        break;

    default:
        break;
    }

    delete rcvMsg;
}

uchar *SerialLink::allocBuffer(int size)
{
    return (uchar *)malloc(sizeof(uchar) * size);
}

void SerialLink::request(int num_params, uchar *payload)
{
    comm->clearSnd();
    payload[0] = requestAckWaitCheck.getNextAckFrameId();
#ifdef DEBUG
    printf("request(): frameId = %d\n", payload[0]);
#endif
    for (int i = 0; i < num_params; i++)
    {
        // printf ("writing payload[%d] = %d\n - buffer: %d\n", i, payload[i], comm->sendDataSize());
        comm->write(payload[i]);
    }

    // lock();
    comm->sendData();
    // unlock();
}

bool SerialLink::syncRequest(int num_params, uchar *payload)
{
    unsigned int time_ms = 0, ack_time_ms = 0;

    while (time_ms < REQUEST_TIMEOUT_ms)
    {
        ack_time_ms = 0;
        request(num_params, payload);

        while (ack_time_ms < ACK_TIMEOUT_ms)
        {
            if (this->requestAckWaitCheck.isAck(payload[0]))
                return true;

            ack_time_ms += wait();
        }
#ifdef DEBUG
        printf("syncRequest(): ACK timeout\n");
#endif
        time_ms += ack_time_ms;
    }

    return false;
}

SerialLink::SerialLink(ISerialCommunication *comm)
{
    this->comm = comm;
    initialize();
}
SerialLink::SerialLink(const char *device)
{
    this->comm = new SerialCommunication(device);
    initialize();
}

SerialLink::~SerialLink()
{
    if (run)
    {
        run = false;
        this->rcvThread->join();
    }
    delete this->comm;
    delete this->rcvThread;

    delete this->handlers;
}

void SerialLink::addHandler(uchar deviceId, uchar handlerId, std::function<void(ResponseData *)> &func)
{
    if ((*this->handlers).find(deviceId) == (*this->handlers).end())
    {
        (*this->handlers)[deviceId] = new std::vector<SerialLinkResponseCallback *>();
    }
    auto vector = ((*this->handlers)[deviceId]);

    SerialLinkResponseCallback *rc = new SerialLinkResponseCallback();
    rc->id = handlerId;
    rc->callback = func;
    vector->push_back(rc);
}

void SerialLink::removeHandler(uchar deviceId, uchar handlerId)
{
    if ((*this->handlers).find(deviceId) == (*this->handlers).end())
        return;

    std::vector<SerialLinkResponseCallback *> *vector = (*this->handlers)[deviceId];

    for (auto it = vector->begin(); it != vector->end(); it++)
    {
        if ((*it)->id == handlerId)
        {
            vector->erase(it);
            delete *it;
            return;
        }
    }
}

bool SerialLink::hasHandler(uchar deviceId, uchar handlerId)
{
    if ((*this->handlers).find(deviceId) == (*this->handlers).end())
        return false;

    std::vector<SerialLinkResponseCallback *> *vector = (*this->handlers)[deviceId];

    for (auto it = vector->begin(); it != vector->end(); it++)
    {
        if ((*it)->id == handlerId)
            return true;
    }

    return false;
}


void SerialLink::clearHandlers()
{
    for (auto it = (*this->handlers).begin(); it != (*this->handlers).end(); it++)
    {
        for (auto it2 = it->second->begin(); it2 != it->second->end(); it2++)
        {
            if (*it2 != nullptr)
            {
                delete *it2;
            }
        }
        delete it->second;
    }
}

bool SerialLink::syncRequest(uchar deviceId)
{
    uchar *payload = allocBuffer(3);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    return syncRequest(3, payload);
}

bool SerialLink::syncRequest(uchar deviceId, uchar val1)
{
    uchar *payload = allocBuffer(4);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    payload[3] = val1;
    return syncRequest(4, payload);
}
bool SerialLink::syncRequest(int deviceId, uchar val1, uchar val2)
{
    uchar *payload = allocBuffer(5);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    payload[3] = val1;
    payload[4] = val2;
    return syncRequest(5, payload);
}
bool SerialLink::syncRequest(int deviceId, uchar val1, uint16_t val2)
{
    uchar *payload = allocBuffer(6);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    uint16p v;
    v.val = val2;
    payload[3] = val1;
    payload[4] = v.bval[0];
    payload[5] = v.bval[1];
    return syncRequest(6, payload);
}

bool SerialLink::syncRequest(int deviceId, uchar val1, uchar val2, uchar val3)
{
    uchar *payload = allocBuffer(6);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    payload[3] = val1;
    payload[4] = val2;
    payload[5] = val3;
    return syncRequest(6, payload);
}

void SerialLink::asyncRequest(uchar deviceId)
{
    uchar *payload = allocBuffer(3);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    request(3, payload);
}
void SerialLink::asyncRequest(uchar deviceId, uchar val1)
{
    uchar *payload = allocBuffer(4);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    payload[3] = val1;
    request(4, payload);
}
void SerialLink::asyncRequest(int deviceId, uchar val1, uchar val2)
{
    uchar *payload = allocBuffer(5);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    payload[3] = val1;
    payload[4] = val2;
    request(5, payload);
}
void SerialLink::asyncRequest(int deviceId, uchar val1, uchar val2, uchar val3)
{
    uchar *payload = allocBuffer(6);
    payload[0] = 0;
    payload[1] = PROTOCOL_FRAME_TYPE_DATA;
    payload[2] = deviceId;
    payload[3] = val1;
    payload[4] = val2;
    payload[5] = val3;
    request(6, payload);
}
