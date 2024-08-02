//
//  TMQPicker.cpp
//  TMQHistory
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//
#include "TMQPicker.h"

USING_TMQ_NAMESPACE

/*
 * Implementation of the virtual method Pick. Three key point:
 * 1. Pick action will start from the end of the shadowIterators always, because the index of the
 *  shadowIterators represent the priority of the tmq message at the same time.
 * 2. If picked a valid message, append it to the history, and read detail from storage.
 * 3. Return true if the invoke is success otherwise return false.
 */
bool TMQPicker::Pick(char *topic, TMQMsg &tmqMsg) {
    if (!topic || !topicStorage) {
        return false;
    }
    for (int i = TMQ_PRIORITY_COUNT - 1; i >= 0; --i) {
        Shadow found;
        if (shadowIterators[i]->Lookup(found)) {
            if (topicHistory) {
                ((TMQHistory *) topicHistory)->Append(found);
            }
            strncpy(topic, found.topic, TMQ_TOPIC_MAX_LENGTH);
            return topicStorage->Read(found, tmqMsg);
        }
    }
    return false;
}

/*
 * Upon destructing, delete the shadow iterator also.
 */
TMQPicker::~TMQPicker() {
    for (int i = 0; i < TMQ_PRIORITY_COUNT; ++i) {
        delete shadowIterators[i];
    }
}

/*
 * Construct the TMQPicker.
 */
TMQPicker::TMQPicker(const char **topics, int len, int type)
        : type(type), topicWatcher(topics, len, true), topicHistory(nullptr),
          topicStorage(nullptr) {
    this->topicStorage = nullptr;
    this->topicHistory = nullptr;
}

// Set the storage
void TMQPicker::SetStorage(IStorage *storage) {
    this->topicStorage = storage;
}

// Set the history
void TMQPicker::SetHistory(IHistory *history) {
    this->topicHistory = history;
}

// Set the rc queue, and create ShadowIterator for the queues.
// The length of the shadow queues is const, defined by TMQ_PRIORITY_COUNT, so we called the shadow
// queues as the priority queues.
void TMQPicker::SetQueues(RCQueue<Shadow> *queues) {
    for (int i = 0; i < TMQ_PRIORITY_COUNT; ++i) {
        // Create ShadowIterator for each RCQueue.
        shadowIterators[i] = new ShadowIterator(&(queues[i]), &topicWatcher, type);
    }
}




