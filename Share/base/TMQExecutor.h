#ifndef TMQ_TMQEXECUTOR_H
#define TMQ_TMQEXECUTOR_H

#include "Defines.h"

TMQ_NAMESPACE

class TMQCallable{
public:
    virtual bool OnExecute(long eid) = 0;
};

enum TMQExecutorState{
    EXECUTOR_STATE_INIT,
    EXECUTOR_STATE_STARTING,
    EXECUTOR_STATE_READY,
    EXECUTOR_STATE_RUNNING,
    EXECUTOR_STATE_WAKING,
    EXECUTOR_STATE_WAITING,
    EXECUTOR_STATE_END,
};

class TMQExecutor {
public:
    virtual TMQExecutorState GetState() = 0;
    virtual bool Wakeup() = 0;
};

TMQ_NAMESPACE_END

#endif // TMQ_TMQEXECUTOR_H