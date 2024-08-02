//
//  TMQDispatcher.cpp
//  TMQDispatcher
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQDispatcher.h"
#include "ThreadExecutor.h"
#include "Shadow.h"
#include "TMQSettings.h"
#include "TMQUtils.h"
#include <cstring>

USING_TMQ_NAMESPACE

// Define the key for the max executor count.
#define TMQ_MAX_EXECUTOR_COUNT      "MAX_EXECUTOR_COUNT"

// Constructor of the TMQDispatcher. In this constructor, we will create the executors and a
// tmq message picker with message type TMQ_MSG_TYPE_DISPATCH.
TMQDispatcher::TMQDispatcher(TMQTopic *tmqTopic, int maxExecutorCount)
        : activeExecutorCount(0), executors(nullptr), stop(false) {
    this->tmqTopic = tmqTopic;
    // If the maxExecutorCount is valid, create executors.
    if (maxExecutorCount > 0) {
        this->maxExecutorCount = maxExecutorCount;
        this->executors = new IExecutor *[maxExecutorCount]{nullptr};
    }
    wakeupCount = 0;
    sentCount = 0;
    receiverIdCounter = 0;
    forceActiveSelf = false;
    picker = tmqTopic->CreatePicker(nullptr, 0, TMQ_MSG_TYPE_DISPATCH);
}

// Destructor of TMQDispatcher.
TMQDispatcher::~TMQDispatcher() {
    // Set stop to true, this can make the method OnExecute return fast as soon as possible.
    stop = true;
    // Release the executors.
    for (int i = 0; i < activeExecutorCount; ++i) {
        delete executors[i];
    }
    delete[] executors;
    wakeupCount = 0;
    sentCount = 0;
    // Destroy the message picker.
    tmqTopic->DestroyPicker(picker);
}

/**
 * Set the max executor count. When the count is 0, the forceActiveSelf will be set to true.
 * That will lead to all message dispatch at activate self mode.
 */
void TMQDispatcher::SetMaxExecutorCount(int count) {
    if (count < 0) {
        return;
    }
    mutex.Lock();
    maxExecutorCount = count;
    // if count is zero, set forceActiveSelf to true, otherwise set to false.
    forceActiveSelf = count == 0;
    mutex.UnLock();
}

/**
 * Get the max executor count.
 */
int TMQDispatcher::GetMaxExecutorCount() const {
    return maxExecutorCount;
}

/**
 * This method has three key logic:
 * 1. Activate the caller itself as the executor to dispatch message.
 * 2. If the executor is not create, or the messages are congested, create a new executor.
 * 3. Send a signal to wakeup the waiting executor.
 */
void TMQDispatcher::Wakeup(bool activeSelf) {
    // Check max executor setting, and set its value.
    String value = TMQSettings::GetInstance()->Get(TMQ_MAX_EXECUTOR_COUNT);
    int max = -1;
    if (TMQUtils::ToInt(value.c_str(), value.Size(), &max) && max != maxExecutorCount) {
        SetMaxExecutorCount(max);
    }
    // Activate itself as the executor, dispatch message immediately.
    if (activeSelf || forceActiveSelf) {
        OnExecute(0);
        return;
    }
    dispatcherMutex.Lock();
    wakeupCount += 1;
    for (int i = 0; i < activeExecutorCount; ++i) {
        // Wake up a executor and return if success.
        if (executors[i]->Wakeup()) {
            dispatcherMutex.UnLock();
            return;
        }
    }
    // Create new executor.
    if (activeExecutorCount < maxExecutorCount
        && (wakeupCount - sentCount > activeExecutorCount * EXECUTOR_WAKE_COUNT)) {
        executors[activeExecutorCount] = new ThreadExecutor(this);
        executors[activeExecutorCount]->Wakeup();
        ++activeExecutorCount;
    }
    dispatcherMutex.UnLock();
}

/**
 * Find the topic receivers and invoke OnRunningReceiver method for each topic and receivers.
 */
bool TMQDispatcher::dispatchMessage(const char *topic, const TMQMsg &msg) {
    TMQId *receiverIds = nullptr;
    TMQSize count = GetTopicReceivers(topic, &receiverIds);
    for (int i = 0; i < count; ++i) {
        // Call receivers of the topic.
        OnRunningReceiver(topic, receiverIds[i], msg);
    }
    delete[] receiverIds;
    return true;
}

/**
 * A loop for picking and sending messages.
 * The stop is true or can not pick message any more, exit the loop.
 */
bool TMQDispatcher::OnExecute(TMQId eid) {
    char topic[TMQ_TOPIC_MAX_LENGTH] = {0};
    int localSentCount = 0;
    TMQMsg tmqMsg;
    // Loop for picking messages by picker.
    while (!stop && this->picker->Pick(topic, tmqMsg)) {
        // Dispatch the topic message.
        dispatchMessage(topic, tmqMsg);
        TMQMsg sentMsg(&(tmqMsg.msgId), sizeof(TMQMsgId));
        // Dispatch a sent message.
        dispatchMessage(TOPIC_SENT, sentMsg);
        localSentCount += 1;
    }
    // Counter the sent message.
    dispatcherMutex.Lock();
    sentCount += localSentCount;
    dispatcherMutex.UnLock();
    return false;
}

/**
 * Find receivers and invoke them. Three key process:
 * 1. Find the topic receives and release the mutex.
 * 2. Invoke the virtual method OnReceive for each receivers.
 * 3. Clear the receiver that has called from the running receivers.
 */
void TMQDispatcher::OnRunningReceiver(const char *topic, TMQId recId, const TMQMsg &msg) {
    TMQDispatcherReceiver *receiver = nullptr;
    if (topic == nullptr) {
        return;
    }
    // Find the topic receivers.
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); ++i) {
        if (strncmp(topic, topicReceivers.Get(i)->GetName(), TMQ_TOPIC_MAX_LENGTH) == 0) {
            for (int j = 0; j < topicReceivers.Get(i)->GetReceivers()->Size(); ++j) {
                if (topicReceivers.Get(i)->GetReceivers()->Get(j)->id == recId) {
                    receiver = topicReceivers.Get(i)->GetReceivers()->Get(j);
                    break;
                }
            }
            if (receiver != nullptr) {
                runningReceivers.Add(receiver);
                break;
            }
        }
    }
    receiverMutex.UnLock();
    // Invoke the receiver and remove it from runningReceivers.
    if (receiver != nullptr) {
        receiver->receiver->OnReceive(&msg);
        receiverMutex.Lock();
        for (int i = 0; i < runningReceivers.Size(); ++i) {
            if (runningReceivers.Get(i)->id == recId) {
                runningReceivers.Remove(i);
                break;
            }
        }
        receiverMutex.UnLock();
    }
}

/**
 * Add a topic receiver. All receivers are organized by the topics. So Add a receiver to a topic
 * should find the topic receiver list at first. Then put this receiver at the end of the list.
 */
TMQId TMQDispatcher::AddReceiver(const char *topic, TMQReceiver *receiver) {
    TMQId receiverId = -1;
    if (topic == nullptr || receiver == nullptr) {
        return receiverId;
    }
    receiverMutex.Lock();
    // Find the receiver list where the new receiver will be put into.
    TMQTopicReceivers *tmqTopicReceivers = nullptr;
    for (int i = 0; i < topicReceivers.Size(); i++) {
        if (strncmp(topic, topicReceivers.Get(i)->GetName(), TMQ_TOPIC_MAX_LENGTH) == 0) {
            tmqTopicReceivers = topicReceivers.Get(i);
            break;
        }
    }
    // If this topic receiver list is not exist, create a new tmq list for it.
    if (tmqTopicReceivers == nullptr) {
        tmqTopicReceivers = new TMQTopicReceivers(topic);
        topicReceivers.Add(tmqTopicReceivers);
    }
    // Put the receiver to the topic receiver list.
    receiverIdCounter += 1;
    receiverId = receiverIdCounter;
    tmqTopicReceivers->GetReceivers()->Add(new TMQDispatcherReceiver(receiver, receiverIdCounter));
    receiverMutex.UnLock();
    return receiverId;
}

/**
 * Remove a receiver from the topic receivers. Attention, remove a running receiver will be
 * time-consumed, because the removing will wait until the receiver running to end.
 */
bool TMQDispatcher::RemoveReceiver(TMQId receiverId) {
    // remove exist receiver
    TMQDispatcherReceiver *dispatcherReceiver = nullptr;
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); i++) {
        for (int j = 0; j < topicReceivers.Get(i)->GetReceivers()->Size(); ++j) {
            if (topicReceivers.Get(i)->GetReceivers()->Get(j)->id == receiverId) {
                dispatcherReceiver = topicReceivers.Get(i)->GetReceivers()->Get(j);
                topicReceivers.Get(i)->GetReceivers()->Remove(j);
                break;
            }
        }
        if (dispatcherReceiver != nullptr) {
            break;
        }
    }
    receiverMutex.UnLock();
    if (dispatcherReceiver == nullptr) {
        return receiverId;
    }
    // wait until running to end
    while (true) {
        bool isRunning = false;
        receiverMutex.Lock();
        for (int i = 0; i < runningReceivers.Size(); ++i) {
            if (dispatcherReceiver == runningReceivers.Get(i)) {
                isRunning = true;
                break;
            }
        }
        receiverMutex.UnLock();
        if (!isRunning) {
            break;
        }
    }
    delete dispatcherReceiver;
    return true;
}

/*
 * Find a receiver from the tmq topic receiver list.
 */
TMQReceiver *TMQDispatcher::FindReceiver(TMQId receiverId) {
    TMQDispatcherReceiver *dispatcherReceiver = nullptr;
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); i++) {
        for (int j = 0; j < topicReceivers.Get(i)->GetReceivers()->Size(); ++j) {
            if (topicReceivers.Get(i)->GetReceivers()->Get(j)->id == receiverId) {
                dispatcherReceiver = topicReceivers.Get(i)->GetReceivers()->Get(j);
                break;
            }
        }
        if (dispatcherReceiver != nullptr) {
            break;
        }
    }
    receiverMutex.UnLock();
    return dispatcherReceiver == nullptr ? nullptr : dispatcherReceiver->receiver;
}

/*
 * Get the receiver ids for a topic. This method will compare all the topic in topicReceivers, and
 * collect the matched receiver id.
 */
TMQSize TMQDispatcher::GetTopicReceivers(const char *topic, TMQId **ids) {
    TMQSize count = 0;
    if (topic == nullptr || ids == nullptr) {
        return count;
    }
    receiverMutex.Lock();
    for (int i = 0; i < topicReceivers.Size(); ++i) {
        if (strncmp(topic, topicReceivers.Get(i)->GetName(), TMQ_TOPIC_MAX_LENGTH) == 0
            && topicReceivers.Get(i)->GetReceivers()->Size() > 0) {
            count = topicReceivers.Get(i)->GetReceivers()->Size();
            *ids = new TMQId[count];
            for (int j = 0; j < count; ++j) {
                (*ids)[j] = topicReceivers.Get(i)->GetReceivers()->Get(j)->id;
            }
            break;
        }
    }
    receiverMutex.UnLock();
    return count;
}


