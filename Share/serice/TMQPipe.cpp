//
//  TMQPipe.cpp
//  TMQPipe
//
//  Created by  on 2022/6/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQPipe.h"
#include <unistd.h>
#include <fcntl.h>
#include "ThreadExecutor.h"
#include "Defines.h"
#include <cstdlib>
#include <climits>

TMQPipe::TMQPipe(const char *pipe, TMQTopic *tmqTopic) : path{0} {

    memset(path, 0, sizeof(path));
    strncpy(path, pipe, sizeof(path));
    path[sizeof(path) - 1] = 0;
    this->tmqTopic = tmqTopic;
    pipeReader = new Pipe(pipe);
    tmqExecutor = new ThreadExecutor(this);
    tmqExecutor->Wakeup();
    signal(SIGPIPE, SIG_IGN);
}

TMQPipe::~TMQPipe() {
    delete tmqExecutor;
    delete pipeReader;
}

bool TMQPipe::OnExecute(long eid) {
    unsigned char queue[PIPE_BUF];
    int start = 0;
    int end = 0;
    int queueLen = sizeof(queue);
    while (true) {
        int left = end >= start ? queueLen - end : start - end;
        LOG_DEBUG("Reading, start:%d, end:%d, left:%d", start, end, left);
        int count = pipeReader->Read(queue + end, left);
        if (count > 0) {
            end += count;
            end = end % queueLen;
        } else {
            continue;
        }
        // detect message
        unsigned char data[PIPE_BUF];
        int dataIndex = 0;
        bool found = false;
        int pos = start;
        int next = (pos + 1) % queueLen;
        while (pos != (queueLen + end - 2) % queueLen && pos != (queueLen + end - 1) % queueLen) {
            if (queue[pos] == 0xff && queue[next] != 0xff) {
                if (found) {
                    data[dataIndex++] = queue[pos];
                }
                found = true;
            } else if (found && queue[pos] == 0x00 && queue[next] == 0x00) {
                data[dataIndex++] = queue[pos];
                data[dataIndex++] = queue[next];
                next = (pos + 2) % queueLen;
            } else if (queue[pos] == 0x00 && queue[next] != 0x00) {
                OnPipeMessage(data, dataIndex);
                found = false;
                start = next;
                dataIndex = 0;
            } else if (found) {
                data[dataIndex++] = queue[pos];
            }
            pos = next;
            next = (pos + 1) % queueLen;
        }
    }
}

bool TMQPipe::OnPipeMessage(unsigned char *origin, int len) {
    if (origin == nullptr || len <= 0) {
        return false;
    }
    unsigned char *decode = nullptr;
    int decodeLen = Pipe::Decode(origin, len, &decode);
    if (decodeLen < 7) {
        return false;
    }
    PMessage message;
    int decodeIndex = 0;
    decodeIndex += Pipe::ReadByte(decode, &message.type);
    decodeIndex += Pipe::ReadShort(decode + decodeIndex, (short *) &message.sender);
    decodeIndex += Pipe::ReadInt(decode + decodeIndex, (int *) &message.mid);
    message.Data(decode + decodeIndex, decodeLen - decodeIndex);
    DispatchMessage(message);
    delete[] decode;
    return true;
}

void TMQPipe::DispatchMessage(const PMessage &message) {
    if (message.len <= 0) {
        return;
    }
    if (message.type == TYPE_LONG_START || message.type == TYPE_LONG_END) {
        longMessages.Add(new PMessage(message));
    } else if (message.type == TYPE_LONG_END) {
        unsigned char *ds;
        unsigned int dsLen = 0;
        List<PMessage *> senderMessages;
        for (int i = 0; i < longMessages.Size(); ++i) {
            PMessage *longMessage = longMessages.Get(i);
            if (longMessage->sender == message.sender && longMessage->mid == message.mid) {
                senderMessages.Add(longMessage);
                dsLen += longMessage->len;
            }
        }
        if (dsLen > 0) {
            ds = (unsigned char *) malloc(dsLen);
            if (ds != nullptr) {
                int dsIndex = 0;
                for (int i = 0; i < senderMessages.Size(); ++i) {
                    memcpy(ds + dsIndex, senderMessages.Get(i)->data, senderMessages.Get(i)->len);
                }
                OnReceive(ds, dsLen);
                free(ds);
            }
        }
        for (int i = 0; i < senderMessages.Size(); ++i) {
            longMessages.Remove(FindLongMessage(senderMessages.Get(i)));
            delete senderMessages.Get(i);
        }

    } else if (message.type == TYPE_MESSAGE) {
        OnReceive(message.data, message.len);
    } else if (message.type == TYPE_REGISTER) {
        OnRegister(message.data, message.len);
    }
    LOG_DEBUG("On Dispatch Message:%d, long message len:%lu", message.mid, longMessages.Size());
}

void TMQPipe::OnReceive(unsigned char *data, unsigned int len) {
    int pos = 0;
    while (pos < len && data[pos++] != '@');
    if (pos <= 1 || pos >= len) {
        return;
    }
    char name[255] = {0};
    memcpy(name, data, pos - 1);
    tmqTopic->Publish(name, data + pos, len - pos);
}

void TMQPipe::OnRegister(unsigned char *info, unsigned int len) {
    LOG_DEBUG("OnRegister, info len:%d", len);
    if (info == nullptr || len <= 1) {
        return;
    }
    char name[255] = {0};
    char pipe[255] = {0};
    int pos = 0;
    while (pos < len && info[pos++] != '@');
    if (pos <= 1 || pos >= len) {
        return;
    }
    memcpy(name, info, pos - 1);
    memcpy(pipe, info + pos, len - pos);
    unsigned short id = -1;
    for (unsigned short index = 0; index < pipeReceivers.Size(); ++index) {
        if (strcmp(pipeReceivers.Get(index)->name, name) == 0
            && strcmp(pipeReceivers.Get(index)->pipe, pipe) == 0) {
            id = index;
            break;
        }
    }
    if (id == (unsigned short) (-1)) {
        auto *receiver = new PipeReceiver((char *) name, (char *) pipe);
        pipeReceivers.Add(receiver);
        id = pipeReceivers.Size() - 1;
        tmqTopic->Subscribe(name, receiver);
    }
    id += 1;
    PMessage message(TYPE_REGISTER, 0, 0);
    unsigned char buf[sizeof(unsigned short)];
    Pipe::WriteShort(buf, (short) id);
    message.Data(buf, sizeof(unsigned short));
    Pipe pw(pipe, false);
    bool success = pw.SendMessage(message);
    LOG_DEBUG("OnRegister, success, name:%s, pipe:%s, id:%d, success:%d", name, pipe, id, success);
}

int TMQPipe::FindLongMessage(const PMessage *pMessage) {
    for (int i = 0; i < longMessages.Size(); ++i) {
        if (longMessages.Get(i) == pMessage) {
            return i;
        }
    }
    return -1;
}


