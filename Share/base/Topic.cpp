// #include "stdio.h"
#include "Topic.h"
#include "TMQFactory.h"
#include "TMQDispatcher.h"
#include "TMQC.h"
#include <cstring>
#include "TMQContext.h"
#include "TMQSettings.h"
#include "TMQUtils.h"

USING_TMQ_NAMESPACE

TMQMsg::TMQMsg() : length(0), data(nullptr), priority(PRIORITY_DEFAULT), flag(0) {}

TMQMsg::TMQMsg(const TMQMsg &tmqMsg)
    : length(0), data(nullptr), priority(PRIORITY_DEFAULT), flag(0)
{
    length = tmqMsg.length;
    if (length > 0)
    {
        data = new char[length];
    }
    if (data == nullptr)
    {
        length = 0;
        return;
    }
    memcpy(data, tmqMsg.data, length);
    priority = tmqMsg.priority;
    flag = tmqMsg.flag;
}
TMQMsg::TMQMsg(const void *data, long length)
    : length(0), data(nullptr), priority(PRIORITY_DEFAULT), flag(0)
{
    if (length <= 0 || data == nullptr)
    {
        return;
    }
    this->data = new char[length];
    if (this->data == nullptr)
    {
        return;
    }
    this->length = length;
    memcpy(this->data, data, length);
}
TMQMsg &TMQMsg::operator=(const TMQMsg &msg)
{
    if (this == &msg)
    {
        return *this;
    }
    priority = msg.priority;
    flag = msg.flag;
    if (data)
    {
        delete[] (char *)data;
        length = 0;
    }
    this->data = new char[msg.length];
    if (this->data != nullptr)
    {
        length = msg.length;
        memcpy(this->data, msg.data, msg.length);
    }
    return *this;
}
TMQMsg::~TMQMsg()
{
    flag = 0;
    length = 0;
    delete[] (char *)data;
}

Topic::Topic() : maxReserved(MAX_RESERVE_DEFAULT)
{
    dispatcher = new TMQDispatcher(this);
    TMQSettings::GetInstance()->Put("version", TMQ_VERSION);
}
Topic::~Topic()
{
    delete dispatcher;
}

long Topic::Publish(const char *topic, void *data, long length, int flag, int priority)
{
    TMQMsg tmqMsg(data, length);
    tmqMsg.flag = flag;
    tmqMsg.priority = priority;
    return Publish(topic, tmqMsg);
}

long Topic::Publish(const char *topic, const TMQMsg &tmqMsg)
{
    if (topic == nullptr || strlen(topic) == 0 || strlen(topic) > TMQ_TOPIC_MAX_LENGTH)
    {
        return TMQ_MSG_ID_INVALID;
    }
    if (tmqMsg.data == nullptr || tmqMsg.length <= 0)
    {
        return TMQ_MSG_ID_INVALID;
    }
    TMsg msg(topic, tmqMsg);
    if (strncmp(topic, TOPIC_SETTINGS, strlen(TOPIC_SETTINGS)) == 0)
    {
        TMQSettings::Parse(static_cast<const char *>(msg.data), msg.length);
        return TMQ_MSG_ID_INVALID;
    }
    msgQueue.Push(msg);
    if (!IS_PICK(msg.flag))
    {
        dispatcher->Wakeup();
    }
    return (long)msg.id;
}

long Topic::Subscribe(const char *topic, TMQReceiver *receiver)
{
    return (long)dispatcher->AddReceiver(topic, receiver);
}
bool Topic::UnSubscribe(long subscribeId)
{
    dispatcher->RemoveReceiver(subscribeId);
    return true;
}
bool Topic::AddSent(const TMsg &msg)
{
    int sentTopicIndex = 0;
    sentMutex.Lock();
    for (; sentTopicIndex < sentMessages.Size(); ++sentTopicIndex)
    {
        if (strncmp(msg.topic, sentMessages.Get(sentTopicIndex).GetName(), TMQ_TOPIC_MAX_LENGTH) == 0)
        {
            if (sentMessages.Get(sentTopicIndex).Size() >= maxReserved)
            {
                sentMessages.Get(sentTopicIndex).Remove(0);
            }
            sentMessages.Get(sentTopicIndex).Add(msg);
            break;
        }
    }
    if (sentTopicIndex == sentMessages.Size())
    {
        TMQList<TMsg> sentTopicMessage;
        sentTopicMessage.SetName(msg.topic);
        sentTopicMessage.Add(msg);
        sentMessages.Add(sentTopicMessage);
    }
    sentMutex.UnLock();
    return true;
}
TMQReceiver *Topic::FindSubscriber(long subscribeId)
{
    return dispatcher->FindReceiver(subscribeId);
}
long Topic::Pick(const char *topic, void **data)
{
    const char *topics[1];
    topics[0] = topic;
    TMsg msg;
    bool success = msgQueue.Pick(topics, 1, msg);
    if (success && msg.length > 0)
    {
        *data = new char[msg.length];
        memcpy(*data, msg.data, msg.length);
    }
    return msg.length;
}
long Topic::Pick(const char **topics, long len, void **data)
{
    TMsg msg;
    bool success = msgQueue.Pick(topics, len, msg);
    if (success && msg.length > 0)
    {
        *data = new char[msg.length];
        memcpy(*data, msg.data, msg.length);
    }
    return msg.length;
}

bool Topic::Poll(TMsg &msg, const char **excludes, int len)
{
    return msgQueue.Poll(msg, excludes, len);
}

TMQMutex instanceMutex;
TMQTopic *TMQFactory::GetTopicInstance()
{
    static TMQTopic *tmqTopic = nullptr;
    if (tmqTopic == nullptr)
    {
        instanceMutex.Lock();
        if (tmqTopic == nullptr)
        {
            tmqTopic = new Topic();
        }
        instanceMutex.UnLock();
    }
    return tmqTopic;
}