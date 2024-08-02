//
//  TMQFactory.cpp
//  TMQFactory
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQFactory.h"
#include "TMQMutex.h"
#include "Topic.h"

USING_TMQ_NAMESPACE
/**
 * TMQMutex for singleton.
 */
TMQMutex tmqInstMutex;

/**
 * Singleton method for achieving the instance of the tmq topic.
 */
TMQTopic *TMQFactory::GetTopicInstance() {
    // A static inner variable for the instance.
    static TMQTopic *tmqTopic = nullptr;
    // Check whether the tmqTopic is nullptr.
    if (tmqTopic == nullptr) {
        tmqInstMutex.Lock();
        // Check whether the tmqTopic is nullptr again.
        if (tmqTopic == nullptr) {
            // Create a instance for tmq topic.
            tmqTopic = new Topic();
        }
        tmqInstMutex.UnLock();
    }
    return tmqTopic;
}
