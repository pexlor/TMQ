#ifndef ANDROID_TMQCONTEXT_H
#define ANDROID_TMQCONTEXT_H

#include "TMQC.h"
#include "TMQTopic.h"
#include "TMQList.h"
#include "TMQMutex.h"

#define CONTEXT_TOPIC_LENGTH 256

class TMQContext : public TMQReceiver
{
private:
    unsigned long lastMsgId = -1;
    TMQMessageCallback callback;

    TMQMutex mutex;
    int length = 0;
    char *topics[CONTEXT_TOPIC_LENGTH] = {0};
    long subscribeIds[CONTEXT_TOPIC_LENGTH] = {0};

public:
    TMQContext(TMQMessageCallback callback)
    {
        this->callback = callback;
    }
    ~TMQContext()
    {
        this->callback = nullptr;
        Remove(nullptr);
    }

    virtual void OnReceive(const TMQMsg *msg);
    bool Add(const char *topic);
    void Remove(const char *topic);
    bool GetTopic(int index, char *topic);
    int GetTopicLen();
    void publish(void *data, long length, int flag, int priority);
    long Pick(void **data);
};

#endif // ANDROID_TMQCONTEXT_H