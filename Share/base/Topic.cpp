//
//  Topic.cpp
//  Topic
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "Topic.h"
#include "TMQFactory.h"
#include "TMQDispatcher.h"
#include "TMQC.h"
#include "MemSpace.h"
#include "Defines.h"
#include "TMQPicker.h"
#include "IDGenerator.h"
#include "TMQSettings.h"
#include <cstring>

#define KEY_TMQ_VERSION     "tmq_version"

USING_TMQ_NAMESPACE

/*
 * Init the base members.
 * In order to record the version of the tmq, we put it into the settings, so that all modules can
 * achieve the tmq version easily.
 */
Topic::Topic() {
    storage = new TMQStorage();
    history = new TMQHistory(storage);
    dispatcher = new TMQDispatcher(this);
    TMQSettings::GetInstance()->Put(KEY_TMQ_VERSION, TMQ_VERSION);
}

/*
 * Sample release all base members.
 */
Topic::~Topic() {
    delete dispatcher;
    delete storage;
    delete history;
}

/*
 * Pack binary data into TMQMsg, and delegate the publish to the tmq message publish function.
 */
TMQMsgId Topic::Publish(const char *topic, void *data, int length, int flag, int priority) {
    TMQMsg tmqMsg(data, length);
    tmqMsg.flag = flag;
    tmqMsg.priority = priority;
    return Publish(topic, tmqMsg);
}

/*
 * Publish a tmq message. Four key steps:
 * 1. Parse and save settings, if the topic is TOPIC_SETTINGS
 * 2. Write the TMQMsg into storage and achieve the shadow.
 * 3. Enqueue TMQMsg into the priority rc queues.
 * 4. Wakeup the dispatcher if it is not pick only message.
 */
TMQMsgId Topic::Publish(const char *topic, const TMQMsg &tmqMsg) {
    // Check the topic
    if (topic == nullptr || strlen(topic) == 0 || strlen(topic) > TMQ_TOPIC_MAX_LENGTH) {
        return ID_LONG_INVALID;
    }
    // Check the TMQMsg.
    if (tmqMsg.data == nullptr || tmqMsg.length <= 0) {
        return ID_LONG_INVALID;
    }
    // Check the topic is TOPIC_SETTINGS or not.
    if (strncmp(topic, TOPIC_SETTINGS, strlen(TOPIC_SETTINGS)) == 0) {
        TMQSettings::Parse(static_cast<const char *>(tmqMsg.data), tmqMsg.length);
        return ID_LONG_INVALID;
    }
    // Write the tmq message to storage
    Shadow msgShadow = storage->Write(topic, tmqMsg);
    if (GET_MSG_TYPE(msgShadow.flag) == 0) {
        msgShadow.flag = FORCE_TYPE_ALL(msgShadow.flag);
    }
    // Enqueue shadow of this message into priority queues.
    FindQueue(tmqMsg.priority)->Enqueue(msgShadow);
    if (msgShadow.flag != TMQ_MSG_TYPE_PICK) {
        // Wake up the dispatcher if necessary.
        dispatcher->Wakeup();
    }
    return msgShadow.msgId;
}

/*
 * Subscribe a topic message by TMQReceiver. Delegating this operation to dispatcher directly.
 */
TMQId Topic::Subscribe(const char *topic, TMQReceiver *receiver) {
    return dispatcher->AddReceiver(topic, receiver);
}

/*
 * UnSubscribe a topic message by subscribeId. Delegating this operation to dispatcher directly.
 */
bool Topic::UnSubscribe(TMQId subscribeId) {
    dispatcher->RemoveReceiver(subscribeId);
    return true;
}

/*
 * Find a topic receiver by subscribeId. Delegating this operation to dispatcher directly.
 */
TMQReceiver *Topic::FindSubscriber(TMQId subscribeId) {
    return dispatcher->FindReceiver(subscribeId);
}

/*
 * Enable persistent storage, invoke EnablePersistent in Storage directly.
 */
bool Topic::EnablePersistent(bool enable, const char *file) {
    if (storage) {
        return EnablePersistent(enable, file);
    }
    return false;
}

/*
 * Create a tmq picker with topics and consuming types.
 */
IPicker *Topic::CreatePicker(const char **topics, int len, int type) {
    auto *picker = new TMQ::TMQPicker(topics, len, type);
    picker->SetHistory(history);
    picker->SetStorage(storage);
    picker->SetQueues(priorityQueue);
    return picker;
}

/**
 * Destroy a picker
 * @param picker, a pointer to the picker.
 */
void Topic::DestroyPicker(IPicker *picker) {
    delete picker;
}

/**
 * Find a priority queue with the priority
 * @param priority, the priority of the queue.
 * @return, a pointer to the RCQueue<Shadow>
 */
RCQueue<Shadow> *Topic::FindQueue(int priority) {
    // Check and reset the index.
    int index = priority < 0 ? 0 : priority;
    index = index >= TMQ_PRIORITY_COUNT ? TMQ_PRIORITY_COUNT - 1 : index;
    return &(priorityQueue[index]);
}

/*
 * Get the history messages, delegate this operation to the history directly.
 */
TMQSize Topic::GetHistory(const char **topics, int len, TMQMsg **values) {
    return history->GetHistory(topics, len, values);
}

/*
 * Get the storage, return directly.
 */
IStorage *Topic::GetStorage() {
    return storage;
}

/*
 * Get the history, return directly.
 */
IHistory *Topic::GetHistory() {
    return history;
}


