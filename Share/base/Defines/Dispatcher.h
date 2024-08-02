//
//  Dispatcher.h
//  Dispatcher
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "Defines.h"

/**
 * An interface definition for message dispatcher. A dispatcher is used to manage all topic receivers,
 * and dispatch the tmq messages. In addition, it has a wakeup function to notify a new message
 * arriving. When there is a new message, you can invoke this function, it will send a signal to the
 * dispatcher. If the dispatcher is running, it will read the message and dispatch it, or if the
 * dispatcher is sleep, it will be woken up after the signal is received.
 */
class Dispatcher {
public:
    /**
     * Wake up the dispatcher when there are new messages arrived.
     * @param activeSelf, a boolean value indicate whether active the caller itself as a thread
     *  executor to dispatch messages. True if you want to dispatch the message immediately, False
     *  indicate that the call of the Wakeup will send a signal to the dispatcher.
     */
    virtual void Wakeup(bool activeSelf = false) = 0;

    /**
     * Add a receiver to subscribe a topic.
     * @param topic, a const pointer to the topic.
     * @param receiver, a pointer to the TMQReceiver.
     * @return the id represent the topic receiver.
     */
    virtual TMQId AddReceiver(const char *topic, TMQReceiver *receiver) = 0;

    /**
     * Remove a topic receiver from the subscription.
     * @param receiverId, the receiverId returned by AddReceiver
     * @return a boolean value indicate whether the remove is success or not.
     */
    virtual bool RemoveReceiver(TMQId receiverId) = 0;

    /**
     * Find a receiver by receiverId
     * @param receiverId, the receiverId returned by AddReceiver
     * @return a pointer to the topic receiver(TMQReceiver)
     */
    virtual TMQReceiver *FindReceiver(TMQId receiverId) = 0;

    /**
     * Virtual destructor for this interface.
     */
    virtual ~Dispatcher() {}
};

#endif //DISPATCHER_H
