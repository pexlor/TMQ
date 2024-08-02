//
//  IDGenerator.cpp
//  IDGenerator
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "IDGenerator.h"
#include "Atomic.h"

// A global static pointer to the instance.
IDGenerator *inst;

/*
 * A method for the IDGenerator singleton, implemented by CAS to handle concurrency issues.
 */
IDGenerator *IDGenerator::GetInstance() {
    if (inst == nullptr) {
        auto *generator = new IDGenerator();
        IDGenerator *local = nullptr;
        // CAS operation to assigned the generator.
        if (!compare_and_set(&inst, &local, &generator)) {
            // CAS fail, instance has been generated, and the real instance has been assigned
            // to local.
            delete generator;
            return local;
        }
    }
    return inst;
}

/*
 * Set the beginning id for the topic id.
 */
void IDGenerator::SetTopicId(TMQId id) {
    // Check the id
    if (id < ID_INT_MIN) {
        return;
    }
    // CAS operation until success.
    TMQId local;
    while (!compare_and_set(&tid, &local, &id));
}

/**
 * Set the beginning id for the message id.
 * @param id
 */
void IDGenerator::SetMsgId(TMQMsgId id) {
    if (id < ID_INT_MIN) {
        return;
    }
    TMQMsgId local = mid;
    while (!compare_and_set(&mid, &local, &id));
}

/**
 * Get a id for topic. if the id increases to the ID_INT_MAX, it will start all over again.
 * @return id for a topic.
 */
TMQId IDGenerator::GetTopicId() {
    TMQId value = add_and_fetch(&tid, ID_INC);
    if (value == ID_INT_MAX) {
        SetTopicId(ID_INT_MIN);
        value = ID_INT_MIN;
    }
    return value;
}

/**
 * Get a id for message. if the id increases to the ID_INT_MAX, it will start all over again.
 * @return id for a topic.
 */
TMQMsgId IDGenerator::GetMsgId() {
    TMQMsgId value = add_and_fetch(&mid, ID_INC);
    if (value == ID_INT_MAX) {
        SetMsgId(ID_INT_MIN);
        value = ID_INT_MIN;
    }
    return value;
}

