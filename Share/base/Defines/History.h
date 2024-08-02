//
//  History.h
//  History
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef HISTORY_H
#define HISTORY_H

#include "Defines.h"
#include "TMQTopic.h"

/**
 * An interface definition for history messages. The history is used to store the messages that have
 * been consumed by pickers or dispatched by the dispatcher. There is only one function in IHistory,
 * that is defined to get the history messages with the specified topics.
 */
class IHistory {
public:
    /**
     * Get the history messages with the specified topics.
     * @param topics, a pointer to the topics pointer.
     * @param len, the length of the topics.
     * @param msg, a pointer to the result messages.
     * @return a TMQSize type value represent the length of the result messages pointed by msg.
     */
    virtual TMQSize GetHistory(const char **topics, int len, TMQMsg **msg) = 0;

    /**
     * Virtual destructor for this interface.
     */
    virtual ~IHistory() {}
};

#endif //HISTORY_H
