//
//  Storage.h
//  Storage
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef STORAGE_H
#define STORAGE_H

#include "TMQTopic.h"
#include "Shadow.h"
#include "List.h"

/**
 * An interface definition for tmq message storage. We make an abstraction of the storage space, and
 * provide three base method: Write, Read and Remove. Write is used to save a message, Read is used
 * to read the tmq message, and Remove is used to delete a tmq message.
 * In additions, we provide a find method called FindShadows to pick the remained shadows that not
 * picked or dispatched timely during past running. This is used to pick the the tmq messages that
 * persisted into a file, which may be the lost messages for power off, or unexpected exit.
 */
class IStorage {
public:
    /**
     * Write a tmq message and return the shadow of this message.
     * @param topic, a const pointer to the topic
     * @param msg, the tmq message to save.
     * @return, Shadow, the shadow of this message
     */
    virtual Shadow Write(const char *topic, const TMQMsg &msg) = 0;

    /**
     * Read a tmq message by a shadow.
     * @param store, the shadow to find tmq message.
     * @param msg, a tmq message refer to save the result.
     * @return bool, a boolean value indicate whether read a message success or not.
     */
    virtual bool Read(const Shadow &store, TMQMsg &msg) = 0;

    /**
     * Remove a tmq message by a shaodw.
     * @param store, the shadow to find tmq message.
     * @return bool, a boolean value indicate whether remove a message success or not.
     */
    virtual bool Remove(const Shadow &store) = 0;

    /**
     * pick the remained shadows that not picked or dispatched timely during the past running.
     * @param topics, a const pointer to the topic pointer.
     * @param len, the length of the topics.
     * @param shadowList, shadowList to save the found result.
     * @param limit, limitation for the find. If the length of shadowList is beyond to the limit the
     *  finding must stop.
     */
    virtual void FindShadows(const char **topics, int len, List<Shadow> &shadowList,
                             int limit = -1) = 0;

    /**
     * Virtual destructor for this interface.
     */
    virtual ~IStorage() {}
};

#endif //STORAGE_H
