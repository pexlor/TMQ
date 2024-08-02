//
//  RWMutex.h
//  RWMutex
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//


#ifndef RWMUTEX_H
#define RWMUTEX_H

#include "TMQMutex.h"

/// Const definition for read-write mutex.
// type for read-write mutex,
// 0: default value, invalid
// 1: share type of the mutex, it allows multiply readers but no writer.
// 2: exclusive type of the mutex, it allows only one writer and no any readers.
#define RW_TYPE_KNOWN       0
#define RW_TYPE_SHARE       1
#define RW_TYPE_EXCLUSIVE   2

/**
 * Read-write mutex, implemented by CAS operation. There is a flag named rw in RWMutex to record the
 * readers and writers. Remember that, it allows multiple readers but only one writer. When the
 * there are readers holding the mutex, the writer must spin for all readers leaving. And when there
 * is a writer, taking the lock for reader will wait until the writer leaving.
 * Read-write mutex is suitable for scene that read more but write less.
 */
class RWMutex {
private:
    // Int value for recording the readers and writer. It is formatted as:
    // {writer}{readers}, where writes is on the high 16 bit and the readers on the low 16 bit.
    int rw;
    // TMQMutex for writer.
    TMQMutex mutex;
public:
    /**
     * Default constructor.
     */
    RWMutex() : rw(0) {

    }

    /**
     * Read lock, return the real type.
     * @return int, a value represent the type of mutex. the return will be RW_TYPE_SHARE or
     *  RW_TYPE_EXCLUSIVE.
     */
    int RLock();

    /**
     * Unlock with mutex type, It should be used as pair with RLock.
     * @param type real type of the mutex, value from RLock.
     */
    void RUnlock(int type);

    /**
     * Lock for writing.
     * @return type of the mutex. only RW_TYPE_EXCLUSIVE will be return.
     */
    int WLock();

    /**
     * Unlock for writing. It should be used as pair with WLock.
     */
    void WUnlock();
};


#endif //RWMUTEX_H
