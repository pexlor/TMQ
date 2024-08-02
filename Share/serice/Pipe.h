//
//  Pipe.h
//  Pipe
//
//  Created by  on 2022/6/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef PIPE_H
#define PIPE_H

#include <stdlib.h>
#include "string.h"

#define TYPE_LONG_START 0xfe
#define TYPE_LONG_END 0xfd
#define TYPE_MESSAGE 0xfc
#define TYPE_REGISTER 0xfb

class PMessage {
public:
    unsigned char type;
    unsigned short sender;
    unsigned int mid;
    unsigned char *data;
    unsigned int len;
    PMessage *next;
public:
    PMessage() : type(TYPE_MESSAGE), sender(0), mid(0), data(nullptr), len(0), next(nullptr) {

    }

    PMessage(unsigned char type, unsigned short sender, unsigned int mid,
             unsigned char *data = nullptr, unsigned int len = 0)
            : type(type), sender(sender), mid(mid), data(nullptr), len(0), next(nullptr) {
        if (data != nullptr && len > 0) {
            this->data = (unsigned char *) malloc(len);
            if (this->data) {
                memcpy(this->data, data, len);
                this->len = len;
            }
        }
    }

    PMessage(const PMessage &message) {
        this->type = message.type;
        this->sender = message.sender;
        this->mid = message.mid;
        this->next = nullptr;
        Data(message.data, message.len);
    }

    void Data(unsigned char *dat, unsigned int length) {
        if (dat == nullptr || length <= 0) {
            return;
        }
        if (this->data != nullptr) {
            free(this->data);
            this->len = 0;
        }
        this->data = (unsigned char *) malloc(length);
        if (this->data) {
            memcpy(this->data, dat, length);
            this->len = length;
        }
    }

    ~PMessage() {
        if (data) {
            free(data);
            len = 0;
        }
        this->next = nullptr;
    }

    static unsigned int GetTokLen(int atomicLength) {
        if (atomicLength <= 0) {
            return -1;
        }
        unsigned int size = sizeof(type) + sizeof(sender) + sizeof(mid);
        unsigned int leftLen = atomicLength - size * 2 - 4;
        return leftLen / 2;
    }
};

class Pipe {
private:
    char *name;
    bool block;
    int fd;
public:
    Pipe(const char *name, bool block = true);

    ~Pipe();

    int Write(const void *buf, int length);

    int Read(void *buf, int length);

    int PSend(const PMessage &msg);

    bool PReceive(PMessage &message);

    int ReceiveMessage(PMessage &message);

    bool SendMessage(const PMessage &message);

public:
    static int GetAtomicLength();

    static int WriteByte(void *buf, unsigned char byte);

    static int ReadByte(void *buf, unsigned char *byte);

    static int WriteShort(void *buf, short value);

    static int ReadShort(void *buf, short *value);

    static int WriteInt(void *buf, int value);

    static int ReadInt(void *buf, int *value);

    static int WriteChars(void *buf, const char *data, int len);

    static int GetEncodeLength(const unsigned char *src, int srcLen);

    static int GetDecodeLength(const unsigned char *src, int srcLen);

    static int Encode(const unsigned char *src, int srcLen, unsigned char **dst);

    static int Decode(const unsigned char *src, int srcLen, unsigned char **dst);

    static int EncodeByte(const unsigned char *src, unsigned char *dst);

    static int DecodeByte(const unsigned char *src, unsigned char *dst);
};


#endif //PIPE_H
