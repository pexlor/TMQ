//
//  TMQContext.cpp
//  TMQContext
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQContext.h"

USING_TMQ_NAMESPACE

TMQContext::TMQContext(TMQMessageCallback messageCallback)
        : lastMsgId(ID_LONG_INVALID), picker(nullptr) {
    this->callback = messageCallback;
}

/**
 * Clear the context.
 * The picker is lazy loaded, so release it when it is not nullptr.
 */
TMQContext::~TMQContext() {
    this->callback = nullptr;
    delete picker;
}

/**
 * Unless the callback is not nullptr and msg.msgId is not equal to the last msg id, the callback
 * will be called. Otherwise, the callback will be ignored.
 */
void TMQContext::OnReceive(const TMQMsg *msg) {
    if (callback != nullptr && msg->msgId != lastMsgId) {
        callback(msg->data, msg->length, (TMQId) this);
        lastMsgId = msg->msgId;
    }
}

// Add a topic to the topic list.
void TMQContext::AddTopic(const char *topic) {
    mutex.Lock();
    // check whether the topic is in the topic list already. If true, give up this adding.
    for (int i = 0; i < topicList.Size(); ++i) {
        if (topicList.Get(i) == topic) {
            // Quick return, unlock the mutex.
            mutex.UnLock();
            return;
        }
    }
    // Add topic safely.
    topicList.Add(topic);
    // Subscribe this topic with the context,
    // and add the subscribe id to subscriberList at the same time.
    subscriberList.Add(TMQFactory::GetTopicInstance()->Subscribe(topic, this));
    // For topic list has being changed, release the picker and assign to nullptr if it exists.
    if (picker) {
        delete picker;
        picker = nullptr;
    }
    mutex.UnLock();
}

/**
 * Remove an exist topic. Firstly, Find the required topic, then remove it, at the same time remove
 * and cancel the subscription . If success, the topics in this context will be changed, so delete
 * the picker and assigned to nullptr.
 */
void TMQContext::RemoveTopic(const char *topic) {
    mutex.Lock();
    for (int i = 0; i < topicList.Size(); ++i) {
        if (topicList.Get(i) == topic) {
            // remove topic
            topicList.Remove(i);
            // cancel the subscription
            TMQFactory::GetTopicInstance()->UnSubscribe(subscriberList.Get(i));
            // remove the subscriber id.
            subscriberList.Remove(i);
            // release picker if it is exist.
            if (picker) {
                delete picker;
                picker = nullptr;
            }
            // finish and break.
            break;
        }
    }
    mutex.UnLock();
}

/**
 * Publish a binary data to this context.
 */
void TMQContext::Publish(void *data, int length, int flag, int priority) {
    mutex.Lock();
    // All topic in this context will be received this publish.
    for (int i = 0; i < topicList.Size(); ++i) {
        TMQFactory::GetTopicInstance()->Publish(topicList.Get(i).c_str(), data, length, flag,
                                                priority);
    }
    mutex.UnLock();
}

/**
 * Pick message from this context.
 */
TMQSize TMQContext::Pick(void **data) {
    mutex.Lock();
    // If the picker has not been created, while the topic list is not empty, we will create the
    // picker first.
    if (!picker && !topicList.Empty()) {
        char **pickerTopics = new char *[topicList.Size()];
        for (int i = 0; i < topicList.Size(); ++i) {
            pickerTopics[i] = (char *) topicList.Get(i).c_str();
        }
        picker = new TMQPicker((const char **) pickerTopics,
                               (int)topicList.Size(), TMQ_MSG_TYPE_ALL);
        delete[] pickerTopics;
    }
    // Check the picker valid before use.
    if (picker) {
        // Stack memory for receive the picked tmq msg topic.
        char pickedTopic[TMQ_TOPIC_MAX_LENGTH] = {0};
        // TMQMsg object for receive message.
        TMQMsg tmqMsg;
        // If the return value is true, and the length of the tmq msg is valid, we assigned the
        // result to *data, and return the real length.
        if (picker->Pick(pickedTopic, tmqMsg) && tmqMsg.length > 0) {
            mutex.UnLock();
            *data = new char[tmqMsg.length];
            memcpy(*data, tmqMsg.data, tmqMsg.length);
            return tmqMsg.length;
        }
    }
    mutex.UnLock();
    // The picker is invalid or pick failed, return MSG_LENGTH_INVALID.
    return MSG_LENGTH_INVALID;
}
