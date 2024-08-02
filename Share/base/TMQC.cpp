#include "TMQTopic.h"
#include "TMQC.h"
#include "cstring"
#include "TMQMutex.h"
#include "TMQList.h"
#include "TMQFactory.h"
#include "TMQContext.h"

USING_TMQ_NAMESPACE

class MessageReceiver : public TMQReceiver
{
public:
    char topic[TMQ_TOPIC_MAX_LENGTH]{0};
    TMQMessageCallback callback;
    long callbackId;

public:
    explicit MessageReceiver(const char *topic, TMQMessageCallback callback, long callbackId = 0) : callback(nullptr), callbackId(0)
    {
        this->callback = callback;
        this->callbackId = callbackId;
        memset(this->topic, 0, TMQ_TOPIC_MAX_LENGTH);
        strcpy(this->topic, topic);
        this->topic[TMQ_TOPIC_MAX_LENGTH - 1] = 0;
    }
    void OnReceive(const TMQMsg *msg) override
    {
        if (callback != nullptr)
            callback(msg->data, msg->length, callbackId);
    }
};
TMQMutex messageReceiverMutex;
TMQList<MessageReceiver *> messageReceivers;
// tmq memory alloc and free
void *tmq_malloc(unsigned long len)
{
    if (len == 0 || len == (unsigned long)(-1))
    {
        return nullptr;
    }
    return new char[len];
}
void tmq_free(void *data)
{
    delete[] (char *)data;
}
// tmq functions
TMQ_EXPORTS long tmq_subscribe(const char *topic, TMQMessageCallback receiver, long customId)
{
    auto *messageReceiver = new MessageReceiver(topic, receiver, customId);
    messageReceiverMutex.Lock();
    messageReceivers.Add(messageReceiver);
    messageReceiverMutex.UnLock();
    return TMQFactory::GetTopicInstance()->Subscribe(topic, messageReceiver);
}
TMQ_EXPORTS bool tmq_unsubscribe(long subId)
{
    auto *messageReceiver = TMQFactory::GetTopicInstance()->FindSubscriber(subId);
    TMQFactory::GetTopicInstance()->UnSubscribe(subId);
    if (messageReceiver != nullptr)
    {
        messageReceiverMutex.Lock();
        messageReceivers.Remove(reinterpret_cast<MessageReceiver *&>(messageReceiver));
        messageReceiverMutex.UnLock();
        delete messageReceiver;
    }
    return true;
}
TMQ_EXPORTS long tmq_pick(const char *topic, void **data)
{
    return TMQFactory::GetTopicInstance()->Pick(topic, data);
}
TMQ_EXPORTS long tmq_pick_with_custom_id(long customId, void **data)
{
    long length = -1;
    messageReceiverMutex.Lock();
    for (int i = 0; i < messageReceivers.Size(); i++)
    {
        if (messageReceivers.Get(i)->callbackId == customId)
        {
            length = TMQFactory::GetTopicInstance()->Pick(messageReceivers.Get(i)->topic, data);
            if (length >= 0)
            {
                break;
            }
        }
    }
    messageReceiverMutex.UnLock();
    return length;
}
TMQ_EXPORTS long tmq_publish(const char *topic, void *data, long length, int flag, int priority)
{
    return TMQFactory::GetTopicInstance()->Publish(topic, data, length, flag, priority);
}

TMQ_EXPORTS long tmq_create_ctx(TMQMessageCallback receiver)
{
    auto *ctx = new TMQContext(receiver);
    return reinterpret_cast<long>(ctx);
}
TMQ_EXPORTS long tmq_ctx_subscribe(long ctx, const char *topic)
{
    auto *topicContext = (TMQContext *)ctx;
    if (topicContext)
    {
        topicContext->Add(topic);
    }
    return reinterpret_cast<long>(topicContext);
}
TMQ_EXPORTS long tmq_ctx_unsubscribe(long ctx, const char *topic)
{
    auto *topicContext = (TMQContext *)ctx;
    if (topicContext)
    {
        topicContext->Remove(topic);
    }
    return reinterpret_cast<long>(topicContext);
}
TMQ_EXPORTS long tmq_ctx_pick(long ctx, void **data)
{
    auto *topicContext = (TMQContext *)ctx;
    if (topicContext)
    {
        return topicContext->Pick(data);
    }
    return -1;
}
TMQ_EXPORTS long tmq_ctx_publish(long ctx, void *data, long length, int flag, int priority)
{
    auto *topicContext = (TMQContext *)ctx;
    topicContext->publish(data, length, flag, priority);
    return reinterpret_cast<long>(topicContext);
}
TMQ_EXPORTS void tmq_destroy_ctx(long ctx)
{
    auto *topicContext = (TMQContext *)ctx;
    if (topicContext)
    {
        topicContext->Remove(nullptr);
        delete topicContext;
    }
}