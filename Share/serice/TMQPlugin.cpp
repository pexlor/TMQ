//
//  TMQPlugin.cpp
//  TMQPlugin
//
//  Created by  on 2022/6/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQPlugin.h"
#include <cstring>
#include "Pipe.h"
#include "Defines.h"
#include <cstdlib>

TMQPlugin::TMQPlugin(const char *name, const char *pipe) {
    memset((void *) this->name, 0, sizeof(this->name));
    memset((void *) this->pipe, 0, sizeof(this->pipe));
    strncpy(this->name, name, sizeof(this->name) - 1);
    strncpy(this->pipe, pipe, sizeof(this->pipe) - 1);
    pluginId = -1;
    mid = -1;
    memset(myPipe, 0, sizeof(myPipe));
}

short TMQPlugin::Register(const char *myPipe) {
    strncpy(this->myPipe, myPipe, sizeof(this->myPipe));
    PMessage message(TYPE_REGISTER, 0, 0);
    message.Data((unsigned char *) myPipe, (unsigned int) strlen(myPipe));
    Pipe writer(pipe, false);
    bool sent = writer.SendMessage(message);
    if (sent) {
        unsigned char *data;
        Receive((void **) &data);
    }
    return pluginId;
}

void TMQPlugin::UnRegister() {
    pluginId = -1;
}

bool TMQPlugin::Send(const char *remote, void *data, int len) {
    if (remote == nullptr || data == nullptr || len <= 0) {
        return false;
    }
    unsigned int rl = (unsigned int) (strlen(remote) + len + 1);
    auto *rd = (unsigned char *) malloc(rl);
    if (rd == nullptr) {
        return false;
    }
    strncpy((char *) rd, remote, rl);
    strcat((char *) rd, "@");
    memcpy(rd + strlen(remote) + 1, data, len);
    mid += 1;
    Pipe pipeWriter(pipe);
    PMessage message(TYPE_MESSAGE, pluginId, mid);
    message.Data(rd, rl);
    pipeWriter.SendMessage(message);
    free(rd);
    return true;
}

int TMQPlugin::Receive(void **data) {
    Pipe pipeReader(myPipe);
    PMessage message;
    int count;
    while ((count = pipeReader.ReceiveMessage(message)) == -1);
    if (count <= 0) {
        return 0;
    }
    if (message.type == TYPE_REGISTER) {
        Pipe::ReadShort(message.data, &pluginId);
        LOG_DEBUG("Plugin receive register data:%d", pluginId);
        return 0;
    }
    *data = malloc(message.len);
    if (*data != nullptr) {
        memcpy(*data, message.data, message.len);
        return (int) message.len;
    }
    return 0;
}


