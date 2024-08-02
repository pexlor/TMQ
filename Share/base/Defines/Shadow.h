//
//  Shadow.h
//  Shadow
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef SHADOW_H
#define SHADOW_H

#include "Metas.h"
#include "Defines.h"
#include "IDGenerator.h"

/// Const definition for the type of the storage.
// Default storage type, invalid type.
#define STORAGE_TYPE_NULL -1
// Memory storage type, indicates that the content is stored in memory space.
#define STORAGE_TYPE_MEMORY 0
// Persistent storage type, indicates that the content is stored in persistent space.
#define STORAGE_TYPE_PERSIST 1

/**
 * Base class for the shadow, that uses to describe the storage types and its address. There are two
 * different type of address, one is the meta info, and the other is the raw data. The meta info
 * usually describe the properties of a tmq message, and the raw data is the binary data of the tmq
 * message. The reason we do take apart to save a message is that the meta info has a constant
 * length, which can improve the storage efficiency, better for quick message searching.
 */
class Store {
public:
    // a tmq address for its meta info.
    TMQAddress metaAddress;
    // a tmq address for its raw data.
    TMQAddress dataAddress;
    // storage type
    int type;

    /**
     * Default constructor, all members are set to invalid value.
     */
    Store() : metaAddress(ADDRESS_NULL), dataAddress(ADDRESS_NULL),
              type(STORAGE_TYPE_NULL) {

    }
};

/**
 * A shadow class for a tmq message. It is based on Store to describe the basic information of a tmq
 * message except its raw data. It is like a shadow, using between TMQTopic, Dispatcher, History and
 * other tmq modules.
 */
class Shadow : public Store {
public:
    // The long id for this shadow(message).
    TMQMsgId msgId;
    // The topic of this shadow(message).
    char topic[TMQ_TOPIC_MAX_LENGTH]{0};
    // The length of this message.
    TMQSize length;
    // The flag of this shadow(message).
    int flag;

public:
    /**
     * Default constructor.
     */
    Shadow() : Store(), length(0), flag(0), msgId(0) {

    }

    /**
     * Default constructor with a topic.
     * @param topic
     */
    explicit Shadow(const char *topic) : Store(), length(0), flag(0), msgId(0) {
        if (topic) {
            // Copy topic
            strncpy(this->topic, topic, sizeof(this->topic));
        }
    }

    /**
     * Construct a Shadow with topic and the tmq message.
     * @param topic, a pointer to the topic.
     * @param tmqMsg, a refer of the tmq message.
     */
    Shadow(const char *topic, const TMQMsg &tmqMsg) : Store(), msgId(0) {
        if (topic) {
            strncpy(this->topic, topic, sizeof(this->topic));
        }
        length = tmqMsg.length;
        flag = tmqMsg.flag;
    }

    /**
     * Override the operator '='
     * @param shadow the source shadow.
     * @return a ref to *this.
     */
    Shadow &operator=(const Shadow &shadow) {
        this->flag = shadow.flag;
        this->msgId = shadow.msgId;
        this->length = shadow.length;
        this->type = shadow.type;
        this->metaAddress = shadow.metaAddress;
        this->dataAddress = shadow.dataAddress;
        strncpy(this->topic, shadow.topic, sizeof(this->topic));
        return *this;
    }
};

#endif //SHADOW_H
