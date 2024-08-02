//
//  TMQC.cpp
//  TMQC
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//


#include "TMQTopic.h"
#include "TMQC.h"
#include "TMQMutex.h"
#include "TMQFactory.h"
#include "Topic.h"
#include "TMQContext.h"
#include <cstring>

TMQ_NAMESPACE
/**
 * A c tmq receiver implemented for c apis. This class contains the topic to subscribe and a
 * TMQMessageCallback function called when there is tmq message with the same topic.
 */
    class TMQCReceiver : public TMQReceiver {
    public:
        // topic for the receiver to subscribe.
        char topic[TMQ_TOPIC_MAX_LENGTH] = {0};
        // c style function for invoking.
        TMQMessageCallback callback;
        // user defined id, a TMQId value or a raw pointer value.
        TMQId callbackId;
    public:
        /*
         * construct for the TMQCReceiver.
         * @param topic, a string pointer to the message topic.
         * @param callback, a function address with type TMQMessageCallback.
         */
        TMQCReceiver(const char *topic, TMQMessageCallback callback, TMQId callbackId = 0) :
                callback(nullptr), callbackId(0) {
            this->callback = callback;
            this->callbackId = callbackId;
            memset(this->topic, 0, TMQ_TOPIC_MAX_LENGTH);
            strncpy(this->topic, topic, sizeof(this->topic));
            this->topic[TMQ_TOPIC_MAX_LENGTH - 1] = 0;
        }

        void OnReceive(const TMQMsg *msg) override {
            if (callback != nullptr)
                callback(msg->data, msg->length, callbackId);
        }
    };
TMQ_NAMESPACE_END

/// C APIs for TMQ.
// namespace defined at first.
USING_TMQ_NAMESPACE
// TMQ c api for memory alloc. This is api for outside, so the type of len is unsigned int,
// as sample as possible.
TMQ_EXPORTS void *tmq_malloc(int len) {
    if (len <= 0) {
        return nullptr;
    }
    return new char[len];
}
// TMQ c api for memory free. Method delete[] can check nullptr itself, so no need to check
// parameter valid. But, if the data is a wild pointer, the behavior will be decided by system.
// So, be care of invalid pointers.
TMQ_EXPORTS void tmq_free(void *data) {
    delete[] (char *) data;
}
//C Api for subscribe a topic, delegating the tmq topic to subscribe.
TMQ_EXPORTS TMQId tmq_subscribe(const char *topic, TMQMessageCallback receiver, TMQId customId) {
    auto *messageReceiver = new TMQCReceiver(topic, receiver, customId);
    return TMQFactory::GetTopicInstance()->Subscribe(topic, messageReceiver);
}
//C Api for unsubscribe a topic, delegating the tmq topic to unsubscribe.
TMQ_EXPORTS bool tmq_unsubscribe(TMQId subId) {
    TMQFactory::GetTopicInstance()->UnSubscribe(subId);
    return true;
}
//C Api for publish a topic message, delegating the tmq topic to publish.
TMQ_EXPORTS TMQMsgId tmq_publish(const char *topic, void *data, int length, int flag, int priority) {
    return TMQFactory::GetTopicInstance()->Publish(topic, data, length, flag, priority);
}
// C Api implementation for creating a tmq context.
TMQ_EXPORTS TMQId tmq_create_ctx(TMQMessageCallback receiver) {
    auto *ctx = new TMQContext(receiver);
    return reinterpret_cast<TMQId>(ctx);
}
// C Api implementation for subscribe message from a tmq context.
TMQ_EXPORTS TMQId tmq_ctx_subscribe(TMQId ctx, const char *topic) {
    auto *topicContext = (TMQContext *) ctx;
    if (topicContext) {
        topicContext->AddTopic(topic);
    }
    return reinterpret_cast<TMQId>(topicContext);
}
// C Api implementation for unsubscribe topic from a tmq context.
TMQ_EXPORTS TMQId tmq_ctx_unsubscribe(TMQId ctx, const char *topic) {
    auto *topicContext = (TMQContext *) ctx;
    if (topicContext) {
        topicContext->RemoveTopic(topic);
    }
    return reinterpret_cast<TMQId>(topicContext);
}
// C Api implementation for enabling or disabling the storage persistence.
TMQ_EXPORTS bool tmq_enable_persistent(bool enable, const char *file) {
    return TMQFactory::GetTopicInstance()->EnablePersistent(enable, file);
}
// C Api implementation for pick messages from tmq context.
TMQ_EXPORTS TMQSize tmq_ctx_pick(TMQId ctx, void **data) {
    auto *topicContext = (TMQContext *) ctx;
    if (topicContext) {
        return topicContext->Pick(data);
    }
    return MSG_LENGTH_INVALID;
}
// C Api implementation for publish messages to tmq context.
TMQ_EXPORTS TMQMsgId tmq_ctx_publish(TMQId ctx, void *data, int length, int flag, int priority) {
    auto *topicContext = (TMQContext *) ctx;
    topicContext->Publish(data, length, flag, priority);
    return reinterpret_cast<TMQId>(topicContext);
}
// C Api implementation for release a tmq context.
TMQ_EXPORTS void tmq_destroy_ctx(TMQId ctx) {
    auto *topicContext = (TMQContext *) ctx;
    delete topicContext;
}