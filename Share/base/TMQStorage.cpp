//
//  TMQStorage.cpp
//  TMQStorage
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQStorage.h"
#include "FileSpace.h"
#include "Shadow.h"
#include "Watcher.h"
#include "TMQBase64.h"

USING_TMQ_NAMESPACE

/*
 * Enable or disable the persistence. During enabling the persistence, TMQStorage will create the
 * persist and some section spaces.
 */
void TMQStorage::EnablePersist(bool enable, const char *file) {
    if (enable && !persist) {
        persistMutex.Lock();
        // Create persistence and some extra section spaces.
        persist = new Persistence(new FileSpace(file));
        dataSpace = persist->CreateLinearSpace(SECTION_DATA);
        metaSpace = persist->CreateLinearSpace(SECTION_META);
        backupSpace = persist->CreateLinearSpace(SECTION_BACKUP);
        // If the meta space is not empty, which means there are remained message, so append these
        // message to backup space and clear meta space.
        persist->AppendLinearSpace(backupSpace->GetName(), metaSpace->GetName());
        persistMutex.UnLock();
    }
    if (!enable && persist) {
        // Give up the persistence and remove all data.
        persistMutex.Lock();
        persist->DropLinearSpace(SECTION_DATA);
        persist->DropLinearSpace(SECTION_TOPIC);
        persist->DropLinearSpace(SECTION_BACKUP);
        persistMutex.UnLock();
        dataSpace = nullptr;
        metaSpace = nullptr;
        delete persist->GetPageSpace();
        delete persist;
        persist = nullptr;
    }
}

/*
 * Write a tmq message. This function will save the tmq message to memory or persistence based on
 * the flag of the tmq message required by user self.
 */
Shadow TMQStorage::Write(const char *topic, const TMQMsg &msg) {
    Shadow shadow(topic, msg);
    // Generate an new id for the tmq message.
    shadow.msgId = IDGenerator::GetInstance()->GetMsgId();
    // The message requires persistent storage, this need the persistence to be available, otherwise
    // the tmq message will save into memory.
    if (IS_PERSIST(msg.flag) && metaSpace && dataSpace) {
        persistMutex.Lock();
        // write binary data into data space, and assign metaAddress into shadow
        int encodeLen = TMQBase64::EncodeLength(msg.length);
        char *encodedBuf = (char *) calloc(encodeLen, sizeof(char *));
        int realEncodeLen = TMQBase64::Encode(encodedBuf, (char *) msg.data, msg.length);
        shadow.dataAddress = dataSpace->Allocate(realEncodeLen);
        dataSpace->Write(shadow.dataAddress, encodedBuf, realEncodeLen);
        if (encodedBuf) {
            free(encodedBuf);
        }
        // write shadow info into meta space
        shadow.metaAddress = metaSpace->Allocate(sizeof(Shadow));
        metaSpace->Write(shadow.metaAddress, (void *) (&shadow), sizeof(Shadow));
        persistMutex.UnLock();
        // Set the storage type to STORAGE_TYPE_PERSIST.
        shadow.type = STORAGE_TYPE_PERSIST;
        shadow.length = realEncodeLen;
    } else {
        // Write the message to the memory.
        auto *memoryAddress = new TMQMsg(msg);
        memoryAddress->msgId = shadow.msgId;
        shadow.metaAddress = (TMQAddress) memoryAddress;
        shadow.dataAddress = (TMQAddress) memoryAddress->data;
        // Set the storage type to STORAGE_TYPE_MEMORY.
        shadow.type = STORAGE_TYPE_MEMORY;
    }
    // Return the shadow of the tmq message.
    return shadow;
}

/*
 * Read a tmq message by the shadow. It will read the tmq message based on the type fo the shadow.
 * Different storage type indicates different type of storage.
 */
bool TMQStorage::Read(const Shadow &shadow, TMQMsg &msg) {
    bool suc = false;
    // The message is saved in persistence.
    if (shadow.type == STORAGE_TYPE_PERSIST && dataSpace) {
        char *base64Buf = (char *) calloc(shadow.length, sizeof(char));
        persistMutex.Lock();
        long len = dataSpace->Read(shadow.dataAddress, base64Buf, shadow.length);
        persistMutex.UnLock();
        int decodedLen = TMQBase64::DecodeLength(base64Buf);
        char *decodedBuf = (char *) calloc(decodedLen, sizeof(char));
        int msgLength = TMQBase64::Decode(decodedBuf, base64Buf);
        msg = TMQMsg(decodedBuf, msgLength);
        free(decodedBuf);
        free(base64Buf);
        suc = len == shadow.length;
    }
    // The message is saved in memory.
    if (shadow.type == STORAGE_TYPE_MEMORY) {
        msg = *((TMQMsg *) shadow.metaAddress);
        suc = true;
    }
    // Set the meta info of the messsage.
    msg.length = shadow.length;
    msg.flag = shadow.flag;
    msg.msgId = shadow.msgId;
    return suc;
}

/**
 * Constructor
 */
TMQStorage::TMQStorage()
        : persist(nullptr), dataSpace(nullptr), metaSpace(nullptr), backupSpace(nullptr) {

}

/*
 * Destructor
 */
TMQStorage::~TMQStorage()
= default;

/*
 * Remove the tmq message by a shadow.
 */
bool TMQStorage::Remove(const Shadow &shadow) {
    // Remove from the persistence.
    if (shadow.type == STORAGE_TYPE_PERSIST && dataSpace && metaSpace) {
        persistMutex.Lock();
        dataSpace->Deallocate(shadow.metaAddress);
        metaSpace->Deallocate(shadow.dataAddress);
        persistMutex.UnLock();
    }
    // Remove from the memory.
    if (shadow.type == STORAGE_TYPE_MEMORY) {
        auto *ptr = (TMQMsg *) shadow.metaAddress;
        delete ptr;
    }
    return true;
}

/*
 * Find the shadow list from backup space.
 */
void TMQStorage::FindShadows(const char **topics, int len, List<Shadow> &shadowList, int limit) {
    // Parameters invalid, return quickly.
    if (!topics || len <= 0 || !dataSpace) {
        return;
    }
    List<MetaAlloc> allocList;
    // Construct a local watcher by topics and its length.
    Watcher localWatcher(topics, len, false);
    persistMutex.Lock();
    // Get all allocation list, and save to allocList
    if (backupSpace->GetAllocList(allocList)) {
        for (int i = 0; i < allocList.Size(); ++i) {
            Shadow shadow;
            // Read and check the topic, if it is a topic message we need, add it into the
            // shadowList.
            backupSpace->Read(allocList.Get(i).address, &shadow, sizeof(Shadow));
            if (localWatcher.Contains(shadow.topic)) {
                shadowList.Add(shadow);
            }
            // Check whether the shadowList reaches to the limit. If reached, stop the loop, and
            // return the results.
            if (limit > 0 && shadowList.Size() > limit) {
                break;
            }
        }
    }
    persistMutex.UnLock();
}
