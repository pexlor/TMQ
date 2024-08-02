
//
//  IDGenerator.h
//  IDGenerator
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include "Defines.h"

/// Const value definitions
// Id increase one by one
#define ID_INC          1
// Max value of the int id.
#define ID_INT_MAX      0xFFFFFFFF
// Min value of the int id, start from 1.
#define ID_INT_MIN      0x00000001

/**
 * Id generator, this class provides common ids using atomic operation.
 */
class IDGenerator {
private:
    // Increased id for topic
    TMQId tid;
    // Increased id for tmq message
    TMQMsgId mid;
public:
    /**
     * Get an id for topic, this will increase tid by one.
     * @return an unsigned int value indicate the topic id.
     */
    TMQId GetTopicId();

    /**
     * Set the topic id
     * @param id, id to be set.
     */
    void SetTopicId(TMQId id);

    /**
     * Get an id for tmq message, this will increase mid by one.
     * @return an unsigned id for tmq message.
     */
    TMQMsgId GetMsgId();

    /**
     * Set the message id.
     * @param id, id to be set.
     */
    void SetMsgId(TMQMsgId id);

    /**
     * Static singleton method for IDGenerator instance.
     * @return a pointer to the IDGenerator instance.
     */
    static IDGenerator *GetInstance();
};


#endif //ID_GENERATOR_H
