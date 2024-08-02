//
//  TMQTypes.cpp
//  TMQTypes
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "Defines.h"
#include "TMQTopic.h"
#include "Topic.h"
#include <cstring>

/*
 * Default construct for tmq message, all members set to zero.
 */
TMQMsg::TMQMsg() : length(0), data(nullptr), priority(0), flag(0), msgId(0) {

}

/*
 * Construct a tmq message with an existed message.
 */
TMQMsg::TMQMsg(const TMQMsg &tmqMsg) : length(0), data(nullptr), priority(0), flag(0), msgId(0) {
    length = tmqMsg.length;
    // Apply memory space and copy the data.
    if (length > 0) {
        data = new char[length];
        memcpy(data, tmqMsg.data, length);
    }
    this->priority = tmqMsg.priority;
    this->flag = tmqMsg.flag;
    this->msgId = tmqMsg.msgId;
}

/*
 * Construct a tmq message with the binary data and its length.
 */
TMQMsg::TMQMsg(const void *data, int length) : length(0), data(nullptr), priority(0), flag(0),
                                                msgId(0) {
    // Check the parameters.
    if (length <= 0 || data == nullptr) {
        return;
    }
    this->data = new char[length];
    this->length = length;
    memcpy(this->data, data, length);
}

/*
 * Override operator =
 */
TMQMsg &TMQMsg::operator=(const TMQMsg &msg) {
    if (this == &msg) {
        return *this;
    }
    this->priority = msg.priority;
    this->flag = msg.flag;
    this->msgId = msg.msgId;
    // Apply memory space and copy data from msg.
    if (msg.length > 0 || msg.data != nullptr) {
        this->data = new char[msg.length];
        this->length = msg.length;
        memcpy(this->data, msg.data, msg.length);
    }
    return *this;
}

/*
 * Destructor of the TMQMsg, reset the members and release the memory space.
 */
TMQMsg::~TMQMsg() {
    flag = 0;
    length = 0;
    msgId = -1;
    delete[] (char *) data;
}

