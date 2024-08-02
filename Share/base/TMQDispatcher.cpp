#include "TMQDispatcher.h"
#include "ThreadExecutor.h"
#include <cstring>

USING_TMQ_NAMESPACE

TMQDispatcher::TMQDispatcher(Topic *topic, int maxExecutorCount)
    : activeExecutorCount(0), executors(nullptr), excludes(nullptr), forceActiveSelf(false)
{
    this->tmqTopic = topic;
    if (maxExecutorCount > 0)
    {
        this->maxExecutorCount = maxExecutorCount;
        this->executors = new TMQExecutor *[maxExecutorCount]
        { nullptr };
        this->excludes = new char *[maxExecutorCount]
        { nullptr };
    }
    wakeupCount = 0;
    sentCount = 0;
    receiverIdCounter = 0;
}
TMQDispatcher::~TMQDispatcher()
{
    for (int i = 0; i < activeExecutorCount; ++i)
    {
        ThreadExecutor::ReleaseExecutor(executors[i]);
    }
    delete[] executors;
    wakeupCount = 0;
    sentCount = 0;
}
void TMQDispatcher::SetMaxExecutorCount(int count)
{
    if (count < 0)
    {
        return;
    }
    mutex.Lock();
    maxExecutorCount = count;
    forceActiveSelf = count == 0;
    mutex.UnLock();
}
int TMQDispatcher::GetMaxExecutorCount()
{
    mutex.Lock();
    int maxCount = maxExecutorCount;
    mutex.UnLock();
    return maxCount;
}
void TMQDispatcher::Wakeup(bool activeSelf)
{
    if (activeSelf || forceActiveSelf)
    {
        OnExecute(0);
        return;
    }
    dispatcherMutex.Lock();
    wakeupCount += 1;
    for (int i = 0; i < activeExecutorCount; ++i)
    {
        if (executors[i]->Wakeup())
        {
            dispatcherMutex.UnLock();
            return;
        }
    }
    if (activeExecutorCount < maxExecutorCount && (wakeupCount - sentCount > activeExecutorCount * EXECUTOR_WAKE_COUNT))
    {
        executors[activeExecutorCount] = ThreadExecutor::CreateExecutor(this);
        executors[activeExecutorCount]->Wakeup();
        ++activeExecutorCount;
    }
    dispatcherMutex.UnLock();
}
bool TMQDispatcher::dispatchMessage(const char *topic, TMsg &msg)
{
    unsigned long *receiverIds = nullptr;
    unsigned long count = GetTopicReceivers(topic, &receiverIds);
    for (int i = 0; i < count; ++i)
    {
        OnRunningReceiver(topic, receiverIds[i], msg);
    }
    delete[] receiverIds;
    return true;
}
bool TMQDispatcher::OnExecute(long eid)
{
    TMsg msg;
    int localSentCount = 0;
    while (tmqTopic->Poll(msg, nullptr, 0))
    {
        dispatchMessage(msg.topic, msg);
        if (strncmp(TOPIC_SENT, msg.topic, strlen(TOPIC_SENT)) != 0)
        {
            tmqTopic->AddSent(msg);
            TMQMsg sentMsg(&(msg.id), sizeof(unsigned long));
            tmqTopic->Publish(TOPIC_SENT, sentMsg);
        }
        localSentCount += 1;
    }
    dispatcherMutex.Lock();
    sentCount += localSentCount;
    dispatcherMutex.UnLock();
    return false;
}
void TMQDispatcher::OnRunningReceiver(const char *topic, unsigned long recId, TMsg &msg)
{
    TMQDispatcherReceiver *receiver = nullptr;
    if (topic == nullptr)
    {
        return;
    }
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); ++i)
    {
        if (strncmp(topic, topicReceivers.Get(i).GetName(), TMQ_TOPIC_MAX_LENGTH) == 0)
        {
            for (int j = 0; j < topicReceivers.Get(i).Size(); ++j)
            {
                if (topicReceivers.Get(i).Get(j)->id == recId)
                {
                    receiver = topicReceivers.Get(i).Get(j);
                    break;
                }
            }
            if (receiver != nullptr)
            {
                runningReceivers.Add(receiver);
                break;
            }
        }
    }
    receiverMutex.UnLock();
    if (receiver != nullptr)
    {
        receiver->receiver->OnReceive(&msg);
        receiverMutex.Lock();
        for (int i = 0; i < runningReceivers.Size(); ++i)
        {
            if (runningReceivers.Get(i)->id == recId)
            {
                runningReceivers.Remove(i);
                break;
            }
        }
        receiverMutex.UnLock();
    }
}
unsigned long TMQDispatcher::AddReceiver(const char *topic, TMQReceiver *receiver)
{
    unsigned long receiverId = -1;
    if (topic == nullptr || receiver == nullptr)
    {
        return receiverId;
    }
    receiverMutex.Lock();
    TMQList<TMQDispatcherReceiver *> *receiverList = nullptr;
    for (int i = 0; i < topicReceivers.Size(); i++)
    {
        if (strncmp(topic, topicReceivers.Get(i).GetName(), MAX_NAME_LENGTH) == 0)
        {
            receiverList = &topicReceivers.Get(i);
            break;
        }
    }
    if (receiverList == nullptr)
    {
        receiverList = new TMQList<TMQDispatcherReceiver *>();
        receiverList->SetName(topic);
        topicReceivers.Add(*receiverList);
        delete receiverList;
        receiverList = nullptr;
        if (topicReceivers.Size() >= 1)
        {
            receiverList = &topicReceivers.Get((long)(topicReceivers.Size() - 1));
        }
    }
    if (receiverList != nullptr)
    {
        receiverIdCounter += 1;
        receiverId = receiverIdCounter;
        receiverList->Add(new TMQDispatcherReceiver(receiver, receiverIdCounter));
    }
    receiverMutex.UnLock();
    return receiverId;
}

unsigned long TMQDispatcher::RemoveReceiver(unsigned long receiverId)
{
    // remove exist receiver
    TMQDispatcherReceiver *dispatcherReceiver = nullptr;
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); i++)
    {
        for (int j = 0; j < topicReceivers.Get(i).Size(); ++j)
        {
            if (topicReceivers.Get(i).Get(j)->id == receiverId)
            {
                dispatcherReceiver = topicReceivers.Get(i).Get(j);
                topicReceivers.Get(i).Remove(j);
                break;
            }
        }
        if (dispatcherReceiver != nullptr)
        {
            break;
        }
    }
    receiverMutex.UnLock();
    if (dispatcherReceiver == nullptr)
    {
        return receiverId;
    }
    // wait until running to end
    while (true)
    {
        bool isRunning = false;
        receiverMutex.Lock();
        for (int i = 0; i < runningReceivers.Size(); ++i)
        {
            if (dispatcherReceiver == runningReceivers.Get(i))
            {
                isRunning = true;
                break;
            }
        }
        receiverMutex.UnLock();
        if (!isRunning)
        {
            break;
        }
    }
    delete dispatcherReceiver;
    return true;
}

TMQReceiver *TMQDispatcher::FindReceiver(unsigned long receiverId)
{
    TMQDispatcherReceiver *dispatcherReceiver = nullptr;
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); i++)
    {
        for (int j = 0; j < topicReceivers.Get(i).Size(); ++j)
        {
            if (topicReceivers.Get(i).Get(j)->id == receiverId)
            {
                dispatcherReceiver = topicReceivers.Get(i).Get(j);
                topicReceivers.Get(i).Remove(j);
                break;
            }
        }
        if (dispatcherReceiver != nullptr)
        {
            break;
        }
    }
    receiverMutex.UnLock();
    return dispatcherReceiver == nullptr ? nullptr : dispatcherReceiver->receiver;
}

unsigned long TMQDispatcher::GetTopicReceivers(const char *topic, unsigned long **ids)
{
    unsigned count = 0;
    if (topic == nullptr || ids == nullptr)
    {
        return count;
    }
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); ++i)
    {
        if (strncmp(topic, topicReceivers.Get(i).GetName(), TMQ_TOPIC_MAX_LENGTH) == 0 && topicReceivers.Get(i).Size() > 0)
        {
            count = topicReceivers.Get(i).Size();
            *ids = new unsigned long[count];
            for (int j = 0; j < count; ++j)
            {
                (*ids)[j] = topicReceivers.Get(i).Get(j)->id;
            }
            break;
        }
    }
    receiverMutex.UnLock();
    return count;
}