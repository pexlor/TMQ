#ifndef ANDROID_TMQTOPIC_H
#define ANDROID_TMQTOPIC_H

#define TMQ_TOPIC_MAX_LENGTH 128
#define TMQ_VERSION "1.1.0"
#define TMQ_MSG_FLAG_PICK 1
#define TMQ_PRIORITY 4

class TMQMsg
{
public:
    long length;
    void *data;
    int priority;
    int flag;

public:
    TMQMsg();
    TMQMsg(const void *data, long length);
    TMQMsg(const TMQMsg &tmqMsg);
    TMQMsg &operator=(const TMQMsg &msg);
    virtual ~TMQMsg();
};

class TMQReceiver
{
public:
    virtual void OnReceive(const TMQMsg *msg) = 0;
    virtual ~TMQReceiver() {};
};

class TMQTopic
{
public:
    virtual long Subscribe(const char *topic, TMQReceiver *receiver) = 0;
    virtual bool UnSubscribe(long subscribeId) = 0;
    virtual TMQReceiver *FindSubscriber(long subscribeId) = 0;
    virtual long Pick(const char *topic, void **data) = 0;
    virtual long Pick(const char **topics, long len, void **data) = 0;
    virtual long Publish(const char *topic, void *data, long length, int flag = 0, int priority = TMQ_PRIORITY) = 0;
    virtual long Publish(const char *topic, const TMQMsg &msg) = 0;
};

#endif // ANDROID_TMQTOPIC_H