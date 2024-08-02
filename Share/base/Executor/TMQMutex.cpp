//
//  TMQMutex.cpp
//  TMQMutex
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TMQMutex.h"

/*
 * Constructor for TMQMutex, init mutex using pthread_mutex_init. We will set the mutex type to
 * PTHREAD_MUTEX_RECURSIVE, which allows to enter a mutex recursively.
 */
TMQMutex::TMQMutex() {
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
#if _WINDOWS
#else
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
#endif
    int r = pthread_mutex_init(&mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);
}

// Lock a mutex, invoke pthread_mutex_lock directly.
bool TMQMutex::Lock() {
    return pthread_mutex_lock(&mutex) == 0;
}

// UnLock a mutex, invoke pthread_mutex_unlock directly.
bool TMQMutex::UnLock() {
    return pthread_mutex_unlock(&mutex) == 0;
}
