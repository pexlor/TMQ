//
//  Queue.h
//  Queue
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef QUEUE_H
#define QUEUE_H

/**
 * An interface definition for tmq message queue, which includes the append and remove apis. This
 * interface is defined to manage the tmq messages. It will be a fifo queue, with the Append method,
 * we can add a tmq message, and the Remove method to delete a tmq message. The address of a message
 * is redefined as QAddress, which has the unsigned long long type, that can save a memory pointer
 * or a persistent address.
 */
// typedef the QAddress
typedef unsigned long long QAddress;

/**
 * The TMQ message queue interface.
 */
class IQueue {
public:
    /**
     * Append a new message to the tail of the queue, and return a QAddress for this message.
     * @param topic, a pointer to the topic.
     * @param tmqMsg, the tmq message to append.
     * @return, a QAddress for this message
     */
    virtual QAddress Append(const char *topic, const TMQMsg &tmqMsg) = 0;

    /**
     * Remove a tmq message by its topic and QAddress
     * @param topic, a const pointer to the topic.
     * @param address, the address of the message.
     * @return, a boolean value indicate whether the remove is success or not.
     */
    virtual bool Remove(const char *topic, QAddress address) = 0;
};

#endif //QUEUE_H
