#ifndef ANDROID_TMQPLUGIN_H
#define ANDROID_TMQPLUGIN_H

class TMQPlugin
{
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

#endif // ANDROID_TMQPLUGIN_H