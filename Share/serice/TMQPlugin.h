//
//  TMQPlugin.h
//  TMQPlugin
//
//  Created by  on 2022/6/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_PLUGIN_H
#define TMQ_PLUGIN_H

class TMQPlugin {
private:
    char name[256];
    char pipe[256];
    char myPipe[256];
    short pluginId;
    unsigned int mid;
public:
    TMQPlugin(const char *name, const char *pipe);

    short Register(const char *myPipe);

    void UnRegister();

    bool Send(const char *remote, void *data, int len);

    int Receive(void **data);
};


#endif //TMQ_PLUGIN_H
