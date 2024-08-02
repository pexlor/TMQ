//
//  ThreadExecutor.h
//  ThreadExecutor
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef THREAD_EXECUTOR_H
#define THREAD_EXECUTOR_H

#include "Executor.h"

#ifndef Executor
#define Executor void*
#endif

class ThreadExecutor : public IExecutor {
private:
    TMQCallable *callable;
    volatile TMQExecutorState state;
public:
    Executor thread;
public:

    explicit ThreadExecutor(const TMQCallable *callable);

    ~ThreadExecutor();

    TMQCallable *GetCallable();

    void SetState(TMQExecutorState state);

public:
    virtual TMQExecutorState GetState();

    virtual bool Wakeup();
};


#endif //THREAD_EXECUTOR_H
