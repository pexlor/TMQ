#include "TMQContext.h"
#include "cstring"
#include "TMQFactory.h"
#include "TMQueue.h"

USING_TMQ_NAMESPACE

void TMQContext::OnReceive(const TMQMsg *msg)
{
    TMsg *tMsg = (TMsg *)msg;
    if (callback != nullptr && lastMsgId != tMsg->id)
    {
        callback(msg->data, msg->length, (long)this);
        lastMsgId = tMsg->id;
    }
}

bool TMQContext::Add(const char *topic)
{
    if (!topic || strlen(topic) >= TMQ_TOPIC_MAX_LENGTH)
    {
        return false;
    }
    mutex.Lock();
    bool exist = false;
    for (int i = 0; i < length; ++i)
    {
        if (strncmp(topic, topics[i], TMQ_TOPIC_MAX_LENGTH) == 0)
        {
            exist = true;
            break;
        }
    }
    if (!exist)
    {
        auto *ptr = new char[strlen(topic) + 1];
        strcpy(ptr, topic);
        ptr[strlen(topic)] = 0;
        topics[length] = ptr;
        if (callback)
        {
            subscribeIds[length] = TMQFactory::GetTopicInstance()->Subscribe(topic, this);
        }
        length++;
    }
    mutex.UnLock();
    return true;
}

void TMQContext::Remove(const char *topic)
{
    if (topic && strlen(topic) > 0 && strlen(topic) < TMQ_TOPIC_MAX_LENGTH)
    {
        mutex.Lock();
        for (int i = 0; i < length; ++i)
        {
            if (strncmp(topic, topics[i], TMQ_TOPIC_MAX_LENGTH) == 0)
            {
                if (subscribeIds[i] > 0)
                {
                    TMQFactory::GetTopicInstance()->UnSubscribe(subscribeIds[i]);
                }
                delete[] topics[i];
                for (int j = i; j + 1 < length; ++j)
                {
                    topics[j] = topics[j + 1];
                    subscribeIds[j] = subscribeIds[j + 1];
                }
                topics[length - 1] = nullptr;
                subscribeIds[length - 1] = 0;
                length--;
                break;
            }
        }
        mutex.UnLock();
        return;
    }
    if (topic == nullptr)
    {
        mutex.Lock();
        for (int i = 0; i < length; ++i)
        {
            if (subscribeIds[i] > 0)
            {
                TMQFactory::GetTopicInstance()->UnSubscribe(subscribeIds[i]);
                subscribeIds[i] = 0;
            }
            delete[] topics[i];
        }
        length = 0;
        mutex.UnLock();
    }
}

bool TMQContext::GetTopic(int index, char *topic)
{
    bool success = false;
    mutex.Lock();
    if (index >= 0 && index < length)
    {
        strcpy(topic, topics[index]);
        success = true;
    }
    mutex.UnLock();
    return success;
}

int TMQContext::GetTopicLen()
{
    int size = 0;
    mutex.Lock();
    size = length;
    mutex.UnLock();
    return size;
}

void TMQContext::publish(void *data, long len, int flag, int priority)
{
    char topic[TMQ_TOPIC_MAX_LENGTH];
    for (int i = 0; i < GetTopicLen(); ++i)
    {
        memset(topic, 0, sizeof(topic));
        if (!GetTopic(i, topic))
            continue;
        TMQFactory::GetTopicInstance()->Publish(topic, data, len, flag, priority);
    }
}

long TMQContext::Pick(void **data)
{
    char *pts[CONTEXT_TOPIC_LENGTH] = {nullptr};
    int len;
    mutex.Lock();
    len = length;
    memcpy(pts, topics, sizeof(topics));
    mutex.UnLock();
    return TMQFactory::GetTopicInstance()->Pick((const char **)(pts), len, data);
}