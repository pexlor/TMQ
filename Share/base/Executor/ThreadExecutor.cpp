//
//  ThreadExecutor.cpp
//  ThreadExecutor
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//


#include "pthread.h"

class TExe {
public:
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
public:
    TExe();

    ~TExe();
};

#define Executor TExe*

#include "ThreadExecutor.h"

ThreadExecutor::ThreadExecutor(const TMQCallable *callable) : thread(nullptr) {
    state = EXECUTOR_STATE_INIT;
    this->callable = const_cast<TMQCallable *>(callable);
    thread = new TExe();
}

ThreadExecutor::~ThreadExecutor() {
    pthread_mutex_lock(&thread->mutex);
    bool wait = state >= EXECUTOR_STATE_STARTING;
    if (state == EXECUTOR_STATE_WAITING) {
        pthread_cond_signal(&thread->cond);
    }
    state = EXECUTOR_STATE_ENDING;
    pthread_mutex_unlock(&thread->mutex);
    while (wait && GetState() != EXECUTOR_STATE_RELEASE);
    delete thread;
}

TMQExecutorState ThreadExecutor::GetState() {
    TMQExecutorState localState;
    pthread_mutex_lock(&thread->mutex);
    localState = state;
    pthread_mutex_unlock(&thread->mutex);
    return localState;
}

void ThreadExecutor::SetState(TMQExecutorState executorState) {
    pthread_mutex_lock(&thread->mutex);
    this->state = executorState;
    pthread_mutex_unlock(&thread->mutex);
}

void *OnExecute(void *executor) {
    auto *threadExecutor = (ThreadExecutor *) executor;
    threadExecutor->SetState(EXECUTOR_STATE_READY);
    while (threadExecutor->GetState() != EXECUTOR_STATE_ENDING) {
        threadExecutor->SetState(EXECUTOR_STATE_RUNNING);
#if _WINDOWS
        threadExecutor->GetCallable()->OnExecute((long)threadExecutor->thread->tid.x);
#else
        threadExecutor->GetCallable()->OnExecute((long) threadExecutor->thread->tid);
#endif
        pthread_mutex_lock(&threadExecutor->thread->mutex);
        if (threadExecutor->GetState() == EXECUTOR_STATE_RUNNING) {
            threadExecutor->SetState(EXECUTOR_STATE_WAITING);
            pthread_cond_wait(&threadExecutor->thread->cond, &threadExecutor->thread->mutex);
        }
        pthread_mutex_unlock(&threadExecutor->thread->mutex);
    }
    threadExecutor->SetState(EXECUTOR_STATE_RELEASE);
    return nullptr;
}

bool ThreadExecutor::Wakeup() {
    if (GetState() >= EXECUTOR_STATE_ENDING) {
        return false;
    }
    bool success = false;
    pthread_mutex_lock(&thread->mutex);
    bool nullThread = false;
#if _WINDOWS
    nullThread = (thread->tid.p == nullptr);
#else
    nullThread = (thread->tid == 0);
#endif
    if (nullThread) {
        state = EXECUTOR_STATE_STARTING;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread->tid, &attr, OnExecute, this);
        pthread_attr_destroy(&attr);
        success = true;
    } else if (GetState() == EXECUTOR_STATE_WAITING) {
        pthread_cond_signal(&thread->cond);
        success = true;
    }
    pthread_mutex_unlock(&thread->mutex);
    return success;
}


TMQCallable *ThreadExecutor::GetCallable() {
    return callable;
}

TExe::TExe() : mutex{0}, cond{0} {
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);
    pthread_cond_init(&cond, nullptr);
#ifdef _WIN32
    tid = {};
#else // _WIN32
    tid = 0;
#endif
}

TExe::~TExe() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}
