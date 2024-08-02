#ifndef ANDROID_TMQDISPATCHER_H
#define ANDROID_TMQDISPATCHER_H

#include "Topic.h"
#include "TMQExecutor.h"
#include "TMQList.h"
#include "TMQMutex.h"
TMQ_NAMESPACE

#define DEFAULT_EXECUTOR_COUNT 4
#define EXECUTOR_WAKE_COUNT 512
#define TMQ_RECEIVER_CODE_FINISH 1

class TMQDispatcherReceiver
{
public:
    TMQReceiver *receiver;
    unsigned long id;

public:
    TMQDispatcherReceiver(TMQReceiver *tmqReceiver, unsigned long id)
    {
        this->receiver = tmqReceiver;
        this->id = id;
    }
};

class TMQDispatcher : public Dispatcher, TMQCallable
{
private:
    Topic *tmqTopic;
    TMQMutex mutex;
    int maxExecutorCount;
    int activeExecutorCount;
    TMQExecutor **executors;
    char **excludes;
    TMQMutex dispatcherMutex;
    // receiver mutex, for topic receivers only
    TMQMutex receiverMutex;
    TMQList<TMQList<TMQDispatcherReceiver *>> topicReceivers;
    // running receivers, for waiting while unsubscribe
    TMQList<TMQDispatcherReceiver *> runningReceivers;

    unsigned long wakeupCount;
    unsigned long sentCount;
    volatile bool forceActiveSelf;
    unsigned long receiverIdCounter;

public:
    TMQDispatcher(Topic *topic, int maxExecutorCount = DEFAULT_EXECUTOR_COUNT);
    ~TMQDispatcher();
    bool OnExecute(long eid);
    void SetMaxExecutorCount(int count);
    int GetMaxExecutorCount();
    void OnRunningReceiver(const char *topic, unsigned long recId, TMsg &msg);
    virtual unsigned long AddReceiver(const char *topic, TMQReceiver *receiver);
    virtual unsigned long RemoveReceiver(unsigned long receiverId);
    virtual TMQReceiver *FindReceiver(unsigned long receiverId);
    virtual void Wakeup(bool activeSelf);
    bool dispatchMessage(const char *topic, TMsg &msg);
    unsigned long GetTopicReceivers(const char *topic, unsigned long **ids);
};

TMQ_NAMESPACE_END

#endif // ANDROID_TMQDISPATCHER_H