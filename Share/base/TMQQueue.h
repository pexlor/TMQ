#ifndef ANDROID_TMQUEUE_H
#define ANDROID_TMQUEUE_H

#include "TMQMutex.h"
#include "LinkList.h"
#include "TMQTopic.h"
#include "cstring"

TMQ_NAMESPACE

#define IS_PICK(flag) (flag & 0x00000001)

#define PRIORITY_LENGTH 10
#define PRIORITY_DEFAULT 4

class TMsg : public TMQMsg
{
public:
    char topic[TMQ_TOPIC_MAX_LENGTH];
    unsigned long id;

public:
    TMsg() : topic{0}, id(-1), TMQMsg() {};
    TMsg(const TMsg &msg) : topic{0}, id(msg.id), TMQMsg(msg)
    {
        strcpy(this->topic, msg.topic);
    }
    TMsg(const char *topic, const TMQMsg &tmqMsg) : topic{0}, id(-1), TMQMsg(tmqMsg)
    {
        strcpy(this->topic, topic);
    }
    TMsg &operator=(const TMsg &msg)
    {
        if (this != &msg)
        {
            id = msg.id;
            memcpy(topic, msg.topic, TMQ_TOPIC_MAX_LENGTH);
            TMQMsg::operator=(msg);
        }
        return *this;
    }
};

class TMQueue
{
private:
    TMQMutex mutex;
    LinkList<TMsg> *queue[PRIORITY_LENGTH];
    unsigned long msgId;
    unsigned long size;

private:
    static bool AllowPoll(const TMsg &msg, const char **excludes, int len);
    static bool Contains(const char *topic, const char **topics, int len);

public:
    TMQueue() : queue{0}, msgId(0), size(0) {};
    void Push(TMsg &tmqMsg);
    bool Pick(const char **topics, int len, TMsg &tMsg);
    bool Poll(TMsg &tMsg, const char **excludes, int len);
    unsigned long Size() const;
};

TMQ_NAMESPACE_END

#endif // ANDROID_TMQUEUE_H