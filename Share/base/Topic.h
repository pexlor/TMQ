#ifndef ANDROID_TOPIC_H
#define ANDROID_TOPIC_H

#if _WINDOWS
#include <stdint.h>
#endif

#include "TMQTopic.h"
#include "TMQList.h"
#include "TMQMutex.h"
#include "TMQueue.h"
#include "Defines.h"

#define TMQ_MSG_ID_INVALID -1
#define TOPIC_SENT "__SENT__"
#define TOPIC_SETTINGS "__SETTINGS__"
#define MAX_RESERVE_DEFAULT 128

TMQ_NAMESPACE

class Dispatcher
{
public:
    virtual void Wakeup(bool activeSelf = false) = 0;
    virtual unsigned long AddReceiver(const char *topic, TMQReceiver *receiver) = 0;
    virtual unsigned long RemoveReceiver(unsigned long receiverId) = 0;
    virtual TMQReceiver *FindReceiver(unsigned long receiverId) = 0;
    virtual ~Dispatcher() {};
};

class Topic : public TMQTopic
{
private:
    TMQueue msgQueue;
    Dispatcher *dispatcher;
    long maxReserved;
    TMQList<TMQList<TMsg>> sentMessages;
    TMQMutex sentMutex;

public:
    virtual long Subscribe(const char *topic, TMQReceiver *receiver);
    virtual bool UnSubscribe(long subscribeId);
    virtual TMQReceiver *FindSubscriber(long subscribeId);
    virtual long Pick(const char *topic, void **data);
    virtual long Pick(const char **topics, long len, void **data);
    virtual long Publish(const char *topic, void *data, long length, int flag, int priority);
    virtual long Publish(const char *topic, const TMQMsg &tmqMsg);

public:
    Topic();
    ~Topic();
    bool Poll(TMsg &msg, const char **excludes, int len);
    bool AddSent(const TMsg &msg);
};

TMQ_NAMESPACE_END

#endif // ANDROID_TOPIC_H