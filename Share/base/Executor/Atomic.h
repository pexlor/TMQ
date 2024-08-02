//
//  Atomic.h
//  Atomic
//
//  Created by  on 2022/2/25.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef ATOMIC_H
#define ATOMIC_H

/**
 * Atomic operations for different platforms.
 */

/**
 * Base function of CAS.
 */
#define compare_and_set(ptr, local, expect)   \
        __atomic_compare_exchange(ptr, local, expect, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define compare_and_set_strong(ptr, local, expect)   \
        __atomic_compare_exchange(ptr, local, expect, false, __ATOMIC_RELAXED, __ATOMIC_ACQ_REL)

/**
 * Atomic operation, add *ptr with value and return the new value.
 */
#define add_and_fetch(ptr, val) \
        __atomic_add_fetch(ptr, val, __ATOMIC_RELAXED)
/**
 * Atomic operation, sub *ptr with value and return the new value.
 */
#define sub_and_fetch(ptr, val) \
        __atomic_sub_fetch(ptr, val, __ATOMIC_RELAXED)

#endif //ATOMIC_H
