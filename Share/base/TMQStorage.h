//
//  TMQStorage.h
//  TMQStorage
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_STORAGE_H
#define TMQ_STORAGE_H

#include "Storage.h"
#include "Persistence.h"
#include "TMQMutex.h"
#include "RWMutex.h"
#include "Ordered.h"
#include "Shadow.h"

/// Const definitions
// data persistent section
#define SECTION_DATA           "DATA"
// topic persistent section
#define SECTION_TOPIC          "TOPIC"
// meta persistent section
#define SECTION_META           "META"
// backup persistent section
#define SECTION_BACKUP         "BACKUP"

/**
 * TMQStorage is a mix storage implementation for IStorage. It can save tmq message to a file called
 * storage persistence or the memory based on the flag set by user.
 *
 * Father more, another important feature is that the TMQStorage can save message flexible.
 * For example, if the left space of memory is low, it can change the mode to the persistence, which
 * may has more storage space. Well, the persistent storage space on disk may be full also, this
 * situation often happens during the downloading for game upgrade. How to deal with this problem?
 * The TMQStorage will save message to memory if the persistence is not available.
 *
 * In short, the TMQStorage try the best to save all topic messages.
 *
 * TMQStorage manages the persistent space and memory space. It writes TMQMsg, and return its shadow
 * for message consuming. A shadow includes the base storage information and meta information of the
 * TMQMsg. From the shadow, we can know the detail address of the tmq message and the storage type
 * information.
 *
 * On persistence, TMQStorage will create three section space: dataSpace, metaSpace, and backupSpace.
 * The dataSpace is used to save the binary data of a tmq message. The metaSpace is used to save the
 * base meta information of the tmq message. Well, the backupSpace is a special section space used
 * to record the lost messages, which are not dispatched or picked on time. Such as power down
 * during game running.
 */
class TMQStorage : public IStorage {
private:
    // The pointer to the persistence implementation.
    Persistence *persist;
    // The mutex for the persist, includes persist, dataSpace, metaSpace, backupSpace.
    TMQMutex persistMutex;
    // The ISectionSpace pointer to DATA section space, using to save binary data of a TMQMsg.
    ISectionSpace *dataSpace;
    // The ISectionSpace pointer to META section space, using to save base information of a TMQMsg.
    ISectionSpace *metaSpace;
    // The ISectionSpace pointer to BACKUP section space, using to record lost messages at last time.
    ISectionSpace *backupSpace;

public:
    /**
     * Default constructor.
     */
    TMQStorage();

    /**
     * Default destructor.
     */
    virtual ~TMQStorage();

    /**
     * API to enable the persistence with a file path.
     * @param enable, a boolean value indicates whether enable the persistence.
     * @param file, a string pointer to the file path. The max length limits to 255.
     * Attentions, if the enable is true, file must be valid, if the file is not exist, this
     * function will create a new file first, if the enable is false, file parameter is valid, or
     * persist is not nullptr, this function will drop all section and erase all data. You can set
     * the enable parameter to true and invoke multiply, but be careful of disable.
     */
    void EnablePersist(bool enable, const char *file);

    /**
     * Save tmq message to the storage and return the shadow of this message.
     * @param topic, a pointer to the message topic.
     * @param msg, the tmq message to write.
     * @return Shadow, the shadow of the tmq message to be written.
     */
    virtual Shadow Write(const char *topic, const TMQMsg &msg);

    /**
     * Read a tmq message with the message shadow. True will be returned if success.
     * @param store, the shadow of this msg.
     * @param msg, a reference for the tmq message, which will be set new data.
     * @return true if read success, false if not exist or some error occurs.
     */
    virtual bool Read(const Shadow &store, TMQMsg &msg);

    /**
     * Remove a tmq message using a shadow.
     * @param store, the shadow of the tmq message.
     * @return, a boolean value indicate whether it is success or not.
     */
    virtual bool Remove(const Shadow &store);

    /**
     * Find shadow list by topics with the amount limit. Attentions, FindShadows will search
     * messages from backupSpace, that means we can get the shadows which are not picked or
     * dispatched timely on the next startup.
     * @param topics, the pointer to topics pointer.
     * @param len, the length of the topics.
     * @param shadowList, the list for store the found results.
     * @param limit, a limit for search.
     *  For parameter limit, the FindShadows has no paging function, so you can deduplicate by
     *  yourself, or remove the shadows before the next search.
     */
    virtual void
    FindShadows(const char **topics, int len, List<Shadow> &shadowList, int limit = -1);
};


#endif //TMQ_STORAGE_H
