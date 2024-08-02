//
//  RCQueue.h
//  RCQueue
//
//  Created by  on 2022/9/18.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef RCQUEUE_H
#define RCQUEUE_H

#include "Atomic.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * This is the implementation for random consuming queue called RCQueue, which is lock-free also.
 * Unlike the common lock-free queue, the RCQueue allows user to consume elements at any position of
 * the queue. Besides, it is thread cooperative on elements clean, which can improve concurrent
 * efficiency very much. Thread cooperation on RCQueue means that the last leaving thread has the
 * responsibility to clean the consumed elements, which can avoid the thread conflicts.
 *
 * Concurrent theory of RCQueue:
 * We use a unsigned int value to record the thread behaviors named state. The state is divided
 * into multiply part:
 *   [Removed flag, 1 bit][Writing flag, 1 bit][Reserved 6 bits][Readers, 8 bit][Remains, 16 bits]
 *      Removed flag: a 1 bit flag indicates whether the node is removed or not.
 *      Writing flag: a 1 bit flag indicates whether the node is on writing.
 *      Readers, a 8 bits flag to record how many readers on this node.
 *      Remains, a 16 bits length space to record haw many iterator remains on the node.
 * When the removed flag is 1, it means this node has been consumed. All token operation on this
 *  node will be fail.
 * When the writing flag is 1, it means this node is owned by only one thread. Reading, Writing,
 *  Leaving Removing on this node will be fail.
 * The readers has 8 bit length, which means there will be 256 readers at the same time. When the
 *  readers is beyond to zero, the writing operation on this node will be fail.
 * The remains has 16 bits length, which means there will be max 65535 iterators stopped on it. When
 *  the remains is not zero, the node will not be allowed to release(delete).
 *
 * Operations on the RCQueue nodes:
 *  Refer, make a reference to the specified node, remains++.
 *  Defer, make a deference to the specified node, remains--.
 *  Lock, set up the writing flag on the specified node, writing flag set to 1.
 *  UnLock, reset the writing flag on the specified node, writing flag set to 0.
 *  SetReading, add the readers by 1 to the specified node. readers++.
 *  UnSetReading, sub the readers by 1 from the specified node. readers--.
 *  Enter, add the remains by 1 to the specified node. remains++.
 *  Leave, sub the remains by 1 from the specified node. remains--.
 * When the writing flag is set to 1, The removed nodes after current node will be cleaned until the
 *  normal nodes.
 * When the remains of a node is zero, it will be released(delete).
 * When the readers is not zero, the pointer of the node should not be changed.
 */
// Debug assertion.
#ifdef DEBUG
#define ASSERT(x) if(!(x))abort()
#else
#define ASSERT(x)
#endif
// typedef for the unsigned int
typedef unsigned int State;
// Bits definitions of the State
#define STATE_INVALID           (State)(-1)
#define CTRL_MASK               0xff000000
#define CTRL_REMOVED_MASK       0x80000000
#define CTRL_WRITING_MASK       0x40000000
#define AMOUNT_READER_MASK      0x00ff0000
#define AMOUNT_REMAIN_MASK      0x0000ffff
#define RC_READER               0x00010000
#define RC_REMAIN               0x00000001
/// Bits operations on the state.
// Check the node is removed or not.
#define IS_REMOVED(state)       (state & CTRL_REMOVED_MASK)
// Check the node is on writing or not.
#define IS_WRITING(state)       (state & CTRL_WRITING_MASK)
// Check whether the thread is the last one to leave.
#define NEED_CLEAR(state)       (state == 2)
// Get the ctrl values.
#define GET_CTRL(state)         (state & CTRL_MASK)
// Get the readers count.
#define GET_READERS(state)      ((state & AMOUNT_READER_MASK) >> 16)
// Get the remains count.
#define GET_REMAINS(state)      (state & AMOUNT_REMAIN_MASK)
// Add a reader.
#define INC_READERS(state)      (state + RC_READER)
// Reduce a reader.
#define DEC_READERS(state)      (state - RC_READER)
// Add remains by 1.
#define INC_REMAINS(state)      (state + RC_REMAIN)
// Reduce remains by 1.
#define DEC_REMAINS(state)      (state - RC_REMAIN)
// Clear the writing flag.
#define CLEAR_WRITING(state)    (state & ~CTRL_WRITING_MASK)
// Clear the removed flag.
#define CLEAR_REMOVED(state)    (state & ~CTRL_REMOVED_MASK)
// Clear the readers.
#define CLEAR_READERS(state)    (state & ~AMOUNT_READER_MASK)
// Set up the writing flag.
#define FORCE_WRITING(state)    (state | CTRL_WRITING_MASK)
// Set up the removed flag.
#define FORCE_REMOVED(state)    (state | CTRL_REMOVED_MASK)

/**
 * RCNode definition.
 * @tparam T
 */
template<typename T>
class RCNode {
public:
    // A pointer to the value.
    T *value;
    // A pointer to the next.
    RCNode *next;
    // The state of this node.
    State state;
public:
    /**
     * Default constructor for the RCNode. Default state is 1, because every node is referred by its
     * previous node.
     */
    RCNode() : next(nullptr), state(1), value(nullptr) {}

    /**
     * Construct RCNode with the value. Default state is 1, because every node is referred by
     * its previous node.
     * @param t, a reference to the value.
     */
    RCNode(const T &t) : next(nullptr), state(1) {
        value = new T(t);
    }
};

/**
 * RCQueue definition and implementation.
 * @tparam T, template parameter.
 */
template<typename T>
class RCQueue {
public:
    // A RCNode pointer to the header.
    RCNode<T> *header;
    // A RCNode pointer to its tail.
    RCNode<T> *volatile tail;
    // Count of the normal values.
    volatile int size;
public:
    /*
     * Construct a empty RCQueue, header is created always.
     */
    RCQueue() : tail(nullptr) {
        header = new RCNode<T>();
        tail = header;
        size = 0;
    }

    /**
     * Enqueue a element. This will always try to move the tail to the new node, and set the next of
     * last tail to this new node. After all of the threads finish, the queue will be linked success.
     * @param t, the new value.
     * @return a pointer to the new value.
     */
    RCNode<T> *Enqueue(const T &t) {
        auto *node = new RCNode<T>(t);
        RCNode<T> *local = tail;
        while (!compare_and_set_strong(&tail, &local, &node));
        RCNode<T> *ln = local->next;
        while (!compare_and_set_strong(&(local->next), &ln, &node));
        add_and_fetch(&size, 1);
        return node;
    }

    /**
     * The elements count. Be aware of that the size is the normal elements count, not the real
     * element count. Because there are removed elements remained on the queue.
     * @return the size of this queue.
     */
    int Size() {
        return size;
    }

    /**
     * CAS operation on node state, it will add the remains by 1.
     * @param node, node to refer.
     * @return, the new state after this operation.
     */
    State Refer(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr || GET_REMAINS(node->state) == AMOUNT_REMAIN_MASK) {
            return STATE_INVALID;
        }
        // CAS to increase the remains.
        State state = node->state;
        State expect;
        do {
            expect = INC_REMAINS(state);
            ASSERT(~GET_REMAINS(expect));
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return expect;
    }

    /**
     * CAS operation on node state, it will reduce the remains by 1.
     * @param node, node to defer.
     * @return the new state after this operation.
     */
    State Defer(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr || GET_REMAINS(node->state) == 0) {
            return (State) (-1);
        }
        // CAS to reduce the remains.
        State state = node->state;
        State expect;
        do {
            ASSERT(GET_REMAINS(state));
            expect = DEC_REMAINS(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return expect;
    }

    /**
     * CAS operation on node state, it will setup the writing flag.
     * @param node, node to lock.
     * @return a boolean value indicate whether the operation is success or not.
     */
    bool Lock(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr) {
            return false;
        }
        // CAS to setup the writing flag, it will try util success.
        State state = node->state;
        State expect;
        do {
            state = CLEAR_WRITING(state);
            state = CLEAR_READERS(state);
            expect = FORCE_WRITING(state);
            ASSERT(IS_WRITING(expect));
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return true;
    }

    /**
    * CAS operation on node state, it will clear the writing flag.
    * @param node, node to unlock.
    * @return a boolean value indicate whether the operation is success or not.
    */
    bool Unlock(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr || !IS_WRITING(node->state)) {
            return false;
        }
        // CAS to clear the writing flag, it will try util success.
        State state = node->state;
        State expect;
        do {
            expect = CLEAR_WRITING(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        ASSERT(!IS_WRITING(expect));
        return true;
    }

    /**
     * CAS operation on node state, it will add the readers by 1.
     * @param node, node to set
     * @return a boolean value indicate whether the operation is success or not.
     */
    bool SetReading(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr) {
            return false;
        }
        // CAS to add the readers flag, it will try util success.
        State state = node->state;
        State expect;
        do {
            state = CLEAR_WRITING(state);
            expect = INC_READERS(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return true;
    }

    /**
     * CAS operation on node state, it will reduce the readers by 1.
     * @param node, node to set
     * @return a boolean value indicate whether the operation is success or not.
     */
    bool UnsetReading(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr || GET_READERS(node->state) == 0) {
            return false;
        }
        // CAS to reduce the readers flag, it will try util success.
        State state = node->state;
        State expect;
        do {
            expect = DEC_READERS(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return true;
    }

    /**
     * CAS operation on node state, it will add the remains by 1.
     * @param node, node to enter
     * @return a boolean value indicate whether the operation is success or not.
     */
    bool Enter(RCNode<T> *node) {
        // Check parameters.
        if (node == nullptr) {
            return false;
        }
        // CAS to add the remains flag, it will try util success.
        State state = node->state;
        State expect;
        do {
            ASSERT(GET_REMAINS(state));
            state = CLEAR_WRITING(state);
            expect = INC_REMAINS(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return true;
    }

    /**
     * Check whether the node needs clean or not. If the node need clean, it will set up the writing
     * flag.
     * @param node, node to lock.
     * @param er, a state reference to receive the new state.
     * @return a boolean value indicate whether it is on writing or not.
     */
    bool LockForClear(RCNode<T> *node, State &er) {
        bool writing = false;
        if (node == nullptr) {
            return writing;
        }
        State state = node->state;
        State expect;
        do {
            // Check if it needs clean.
            if (NEED_CLEAR(state)) {
                expect = FORCE_WRITING(state);
                expect = DEC_REMAINS(expect);
                writing = true;
            } else {
                // No need clean, reduce the remains directly.
                ASSERT(GET_REMAINS(state));
                state = CLEAR_WRITING(state);
                expect = DEC_REMAINS(state);
                writing = false;
            }
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        er = expect;
        return writing;
    }

    /**
     * Clear the writing flag if needed.
     * @param node, node to Unlock.
     * @return a boolean value indicate whether the operation is success or not.
     */
    bool UnlockForClear(RCNode<T> *node) {
        if (node == nullptr || !IS_WRITING(node->state)) {
            return false;
        }
        State state = node->state;
        State expect;
        do {
            expect = CLEAR_WRITING(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return true;
    }

    /**
     * CAS operation on node state, it will reduce the remains by 1. When the node is a normal node
     * and it is the last one to leave, it will lock the node and clean the removed element after
     * this node.
     * @param node, node to leave.
     * @return a boolean value indicate whether the operation is success or not.
     */
    bool Leave(RCNode<T> *node) {
        while (node && GET_REMAINS(node->state) > 0) {
            State expect;
            if (LockForClear(node, expect)) {
                while (node->next && (IS_REMOVED(node->next->state))) {
                    Clear(node);
                }
                UnlockForClear(node);
            }
            if (GET_REMAINS(expect) == 0) {
                RCNode<T> *isolated = node;
                node = node->next;
                delete isolated->value;
                delete isolated;
            } else {
                break;
            }
        }
        return true;
    }

    /**
     * Clear elements after the node. Some important actions:
     * 1. When the last node to clean is the tail, we should move the tail to the current node.
     * 2. If the last node to clean is the tail, but move the tail fail, which means there are new
     *  elements add to the queue, we had to wait the new element enqueued.
     * 3. When the remains of the removed elements during the clean is zero, it will be
     *  released(delete) immediately.
     * @param node node to do the clear.
     * @return, a boolean value indicate whether the operation is success or not.
     */
    bool Clear(RCNode<T> *node) {
        // Check parameter.
        ASSERT(IS_WRITING(node->state));
        RCNode<T> *next = node->next;
        if (!next || !(IS_REMOVED(next->state))) {
            return false;
        }
        // Search the continuous removed elements.
        RCNode<T> *seek = next;
        while (seek->next && (IS_REMOVED(seek->next->state))) {
            seek = seek->next;
        }
        // Check the last node is the tail. If it is the tail, we must move the tail or wait for the
        // new tail. It is the concurrency control between the consumer and the producer, very hard
        // to imagine, but the codes is bellow.
        if (seek == tail) {
            RCNode<T> *local = seek;
            if (compare_and_set_strong(&tail, &local, &node)) {
                RCNode<T> *ex = nullptr;
                local = next;
                if (!compare_and_set_strong(&(node->next), &local, &ex)) {
//                    LOG_DEBUG("RCQueue Set tail fail, node:%p, next:%p, seek:%p", node, next, seek);
                }
            } else {
                while (!(seek->next));
                node->next = seek->next;
            }
        } else {
            while (!(seek->next));
            node->next = seek->next;
        }
        // change the next of the last node.
        Lock(seek);
        RCNode<T> *local = seek->next;
        while (!compare_and_set_strong(&(seek->next), &local, &node));
//        seek->next = node;
        Refer(node);
        Unlock(seek);
        // Loop to release elements whose remains is zero.
        while (next && GET_REMAINS(Defer(next)) == 0) {
            // node is the isolated, delete it.
            RCNode<T> *isolated = next;
            next = next->next;
            isolated->next = nullptr;
            delete isolated->value;
            delete isolated;
        }
        return true;
    }

    /**
     * CAS operation on node state to consume a node, it will setup the removed flag.
     * @param node, node to consume.
     * @return, a boolean value indicate whether the take is success or not.
     */
    bool Take(RCNode<T> *node) {
        // Check parameter
        if (node == nullptr) {
            return false;
        }
        // CAS to setup the removed flag, it will try util success.
        State state = node->state;
        State expect;
        do {
            // If the node is consumed, give up the take operation and return false.
            if (IS_REMOVED(state)) {
                return false;
            }
            state = CLEAR_WRITING(state);
            state = CLEAR_REMOVED(state);
            expect = FORCE_REMOVED(state);
        } while (!compare_and_set_strong(&(node->state), &state, &expect));
        return true;
    }
};

/**
 * RCIterator to consume elements on the RCQueue. One iterator will add a remain on node and reduce
 * a remain when leaving that node.
 * @tparam T, template parameter.
 */
template<typename T>
class RCIterator {
public:
    // a pointer to the RCQueue.
    RCQueue<T> *queue;
    // a pointer to the current node which the iterator stopped on.
    RCNode<T> *ptr;
    // a pointer to the next node which the iterator stopped on and prepare to consume.
    RCNode<T> *next;

public:
    /**
     * Default constructor fot the RCIterator.
     */
    RCIterator() : queue(nullptr), ptr(nullptr), next(nullptr) {

    }

    /**
     * Construct a RCIterator withe the RCQueue.
     * @param queue
     */
    RCIterator(RCQueue<T> *queue) : queue(queue), ptr(nullptr), next(nullptr) {

    }

    /**
     * Default destructor. When this iterator destructs, it means leave the RCQueue at the same time.
     * So invoke the leave method before the destruction finish.
     */
    virtual ~RCIterator() {
        // leave current node if ptr is not nullptr.
        if (ptr) {
            queue->Leave(ptr);
        }
        // leave next node if next is not nullptr.
        if (next) {
            queue->Leave(next);
        }
    }

    /**
     * A virtual function to compare the element in node.
     * @param t, the node in RCQueue.
     * @return, true if the iterator consume this node, false if it do not consume it.
     */
    virtual bool OnCompare(const T &t) {
        return false;
    }

    /**
     * Lookup action to consume the RCQueue. Lookup will traverse the RCQueue and search the node by
     * the compare function. If finding a node success, it will take it and return the value. But it
     * will remain on the queue for next search which can improve the searching efficiency.
     * @param val, a value reference to save the result.
     * @return, a boolean value indicate whether we had found a value or not.
     */
    bool Lookup(T &val) {
        // Check parameter
        if (queue == nullptr) {
            return false;
        }
        // Enter the header, the header is not nullptr always.
        if (ptr == nullptr) {
            queue->Enter(queue->header);
            ptr = queue->header;
        }
        ASSERT(GET_REMAINS(ptr->state));
        // Loop for finding the node.
        while (true) {
            // Move and enter the next element.
            if (next == nullptr) {
                queue->SetReading(ptr);
                next = ptr->next;
                if (!queue->Enter(next)) {
                    next = nullptr;
                    queue->UnsetReading(ptr);
                    break;
                }
                queue->UnsetReading(ptr);
            }
            // The next node has been consumed, continue the loop.
            if (IS_REMOVED(next->state)) {
                queue->Leave(ptr);
                ptr = next;
                next = nullptr;
                continue;
            }
            // Check the value of the next and compare if it is the node required, if it is what we
            // want, use the take to consume it. If the take is success, it means that we have
            // consume a node success, return the values.
            if (next->value && OnCompare(*(next->value)) && queue->Take(next)) {
                val = *(next->value);
                sub_and_fetch(&(queue->size), 1);
                return true;
            } else {
                // Enter next and leave the current node.
                queue->Leave(ptr);
                ptr = next;
                next = nullptr;
            }
        }
        // The traverse on queue has been finished, and no consumed node, this lookup finishes with
        // fail, return false as the result, which indicate the traverse has reached to the end.
        return false;
    }
};

#endif //RCQUEUE_H
