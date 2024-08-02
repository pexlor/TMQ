//
//  TMQContext.h
//  TMQContext
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_CONTEXT_H
#define TMQ_CONTEXT_H

#include "Defines.h"
#include "TMQTopic.h"
#include "TMQC.h"
#include "TMQMutex.h"
#include "TMQFactory.h"
#include "TMQPicker.h"
#include "List.h"
#include "Chars.h"

TMQ_NAMESPACE
/**
 * A tmq context can contain one or more topics, and a message callback. It can pick and receive
 * multiple messages with different topic. Publishing messages to a tmq context is equivalent to
 * publish the message to every topic in context. At the same time, one message with the same msgId will
 * trigger the callback only once.
 */
    class TMQContext : public TMQReceiver {
    private:
        // c style message callback, used to receive dispatcher message.
        TMQMessageCallback callback;
        // Mutex for members of the context.
        TMQMutex mutex;
        // Topic list include in this context.
        List<String> topicList;
        // Subscribe id list return by subscribe.
        List<TMQId> subscriberList;
        // Tmq message id for last tmq msg called with callback. This id is used to prevent repeated
        // callbacks with the same msgId.
        TMQMsgId lastMsgId;
        // Picker pointer for picking the messages with the topics included in this context.
        IPicker *picker;

    public:
        /**
         * Default constructor for tmq context.
         * @param messageCallback, The type of TMQMessageCallback c style function for message callback
         */
        TMQContext(TMQMessageCallback messageCallback = nullptr);

        /**
         * Destructor for this context.
         */
        ~TMQContext();

        /*
         * Implement for TMQReceiver(base), used to receive dispatcher message.
         */
        void OnReceive(const TMQMsg *msg) override;

        /**
         * Add a topic to this context. It also means subscribe for this topic at the same time.
         * @param topic, const pointer to the topic.
         * @return void.
         */
        void AddTopic(const char *topic);

        /**
         * Remove a topic from this context, Unsubscribe this topic at the same time.
         * @param topic, const pointer to the topic.
         * @return void.
         */
        void RemoveTopic(const char *topic);

        /**
         * Publish binary data into tmq.
         * @param data, a pointer for the data
         * @param length, the length of the data
         * @param flag, flag for this msg, refer to TMQ_MSG_TYPE_XXX for more detail.
         * @param priority, message priority, default value is PRIORITY_NORMAL.
         * @return void.
         */
        void Publish(void *data, int length, int flag, int priority = PRIORITY_NORMAL);

        /**
         * Pick tmq messages from this context. If success, the data pinter will be assigned to a valid
         * memory address, and the length of this data will be returned.
         * @param data, a pointer to the data pointer.
         * @return the length of this data.
         */
        TMQSize Pick(void **data);
    };

TMQ_NAMESPACE_END

#endif //TMQ_CONTEXT_H
