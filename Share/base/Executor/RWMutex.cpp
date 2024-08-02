//
//  RWMutex.cpp
//  RWMutex
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "RWMutex.h"
#include "Atomic.h"

/*
 * Lock for reading. if there is no writer, it will success. Otherwise it will wait for the writer
 * leaving.
 */
int RWMutex::RLock() {
    bool suc = false;
    int local = rw;
    // Loop until add one to readers success.
    while (!suc) {
        // There is a writer, so lock with TMQMutex, which will wait by sleep.
        if ((local & 0x0000ffff) > 0) {
            mutex.Lock();
            return RW_TYPE_EXCLUSIVE;
        }
        int expect = ((local >> 16) + 1) << 16 | (local & 0x0000ffff);
        suc = compare_and_set(&rw, &local, &expect);
    }
    return RW_TYPE_SHARE;
}

/*
 * Unlock mutex with a type. If the type is RW_TYPE_SHARE, it will sub 1 on readers, or invoke the
 * UnLock of the TMQMutex.
 */
void RWMutex::RUnlock(int type) {
    // Check the rw and type parameter. If it is RW_TYPE_SHARE type, minus 1 on readers.
    if (rw && type == RW_TYPE_SHARE) {
        int local = rw;
        int expect;
        do {
            expect = ((local >> 16) - 1) << 16 | (local & 0x0000ffff);
        } while (!compare_and_set(&rw, &local, &expect));
    }
    // If the type is RW_TYPE_EXCLUSIVE, invoke UnLock of TMQMutex directly.
    if (rw && type == RW_TYPE_EXCLUSIVE) {
        mutex.UnLock();
    }
}

/*
 * Write lock.
 */
int RWMutex::WLock() {
    int local = rw;
    int expect = 0;
    do {
        expect = (local & 0xffff0000) | ((local & 0x0000ffff) + 1);
    } while (!compare_and_set(&rw, &local, &expect));
    // Wait for all reader leaving.
    while (rw >> 16 != 0);
    mutex.Lock();
    return 0;
}

/*
 * Unlock for writer.
 */
void RWMutex::WUnlock() {
    int local = rw;
    int expect = 0;
    do {
        expect = (local & 0xffff0000) | ((local & 0x0000ffff) - 1);
    } while (!compare_and_set(&rw, &local, &expect));
    // Invoke TMQMutex for unlocking.
    mutex.UnLock();
}

