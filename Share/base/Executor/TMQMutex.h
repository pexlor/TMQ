#ifndef ANDROID_TMQMUTEX_H
#define ANDROID_TMQMUTEX_H

#include "pthread.h"
#define Mutex pthread_mutex_t

#ifndef Mutex
#define Mutex void *
#endif

class TMQMutex
{
private:
    Mutex mutex;

public:
    TMQMutex();
    bool Lock();
    bool UnLock();
};

#endif // ANDROID_TMQMUTEX_H