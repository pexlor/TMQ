//
//  TMQMutex.h
//  TMQMutex
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_MUTEX_H
#define TMQ_MUTEX_H

#include "pthread.h"

/// Implementation of mutex for different platforms.
// pthread is a cross-platform implementation for thread, so we use pthread_mutex_t to implement the
// TMQMutex for cross platform directly.
#define Mutex pthread_mutex_t
#ifndef Mutex
#define Mutex void*
#endif

/**
 * TMQMutex provides Lock and Unlock methods for mutex operations.
 */
class TMQMutex {
private:
    // Mutex for platform.
    Mutex mutex{};
public:
    /*
     * Default constructor
     */
    TMQMutex();

    /**
     * Lock
     * @return bool, a boolean value indicate whether the lock is success or not.
     */
    bool Lock();

    /**
     * Unlock
     * @return bool, a boolean value indicate whether the unlock is success or not.
     */
    bool UnLock();
};


#endif //TMQ_MUTEX_H
