#ifndef ANDROID_TMQPIPE_H
#define ANDROID_TMQPIPE_H
#include "TMQExecutor.h"
#include "Pipe.h"
#include "TMQTopic.h"
#include "TMQList.h"

#define TYPE_LONG_START 0xfe
#define TYPE_LONG_END 0xfd
#define TYPE_MESSAGE 0xfc
#define TYPE_REGISTER 0xfb

USING_TMQ_NAMESPACE

class PipeReceiver : public TMQReceiver
{
public:
    char name[255]{0};
    char pipe[255]{0};
    PipeReceiver(const char *name, const char *pipe)
    {
        memset((void *)this->name, 0, sizeof(this->name));
        memset((void *)this->pipe, 0, sizeof(this->pipe));
        memcpy(this->name, name, strlen(name));
        memcpy(this->pipe, pipe, strlen(pipe));
    }
    void OnReceive(const TMQMsg *msg) override
    {
        Pipe pipeWriter(this->pipe);
        PMessage message(TYPE_MESSAGE, 0, 0);
        message.Data((unsigned char *)msg->data, msg->length);
        pipeWriter.SendMessage(message);
    }
};

class TMQPipe : TMQCallable
{
private:
    char path[256];
    TMQTopic *tmqTopic;
    TMQExecutor *tmqExecutor;
    Pipe *pipeReader;
    TMQList<PMessage *> longMessages;
    TMQList<PipeReceiver *> pipeReceivers;

public:
    TMQPipe(const char *pipe, TMQTopic *tmqTopic = nullptr);
    ~TMQPipe();
    bool OnExecute(long eid);

public:
    bool OnPipeMessage(unsigned char *origin, int len);
    void DispatchMessage(const PMessage &message);
    void OnReceive(unsigned char *data, unsigned int len);
    void OnRegister(unsigned char *name, unsigned int len);
};

#endif // ANDROID_TMQPIPE_H