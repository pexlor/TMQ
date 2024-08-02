//
// Created by 大老鼠 on 2022/5/21.
//
#ifndef ANDROID_TQUEUE_H
#define ANDROID_TQUEUE_H
#include <cstring>
#include "Ordered.h"
#include "TMQTopic.h"
#include "Metas.h"
#include "Atomic.h"
#include "RWMutex.h"
#define SHRINK_MIN                  20
#define SHRINK_THR                  0.3
#define ALLOW_CLEAR(val)            (val == 0x80000000)
#define IS_REMOVE(val)             ((val & 0x80000000) != 0)
#define FORCE_REMOVE(val)           (val | 0x80000000)
#define OCCUPY(val, count)          (val & 0x80000000) | ((val & 0x7fffffff) + count)
template <typename T>
class RWNode{
public:
    T t;
    RWNode<T>* next;
    int occupied;
public:
    RWNode(): next(nullptr), occupied(0){};
    RWNode(const T& t): next(nullptr), occupied(0)
    {
        this->t = t;
    };
};
template <typename T>
class RWQueue{
public:
    RWNode<T>* header;
    RWNode<T>* tail;
    int total;
    int removed;
    RWMutex mutex;
    RWQueue();
public:
    bool Append(T &t);
    bool Remove();
    bool IsExceeded();
    bool Occupy(RWNode<T>* node, int count);
    bool Remove(RWNode<T>* node);
};
template<typename T>
RWQueue<T>::RWQueue(): header(nullptr), tail(nullptr), total(0), removed(0){}
template<typename T>
bool RWQueue<T>::Append(T &t)
{
    auto* node = new RWNode<T>(t);
    RWNode<T>* local = tail;
    if (!local && compare_and_set(&tail, &local, &node))
    {
        local = header;
        compare_and_set(&header, &local, &node);
        return true;
    }
    RWNode<T> *tailNext = nullptr;
    while (!compare_and_set(&(local->next), &tailNext, &node))
    {
        local = tailNext;
        tailNext = nullptr;
    }
    compare_and_set(&tail, &local, &node);
    add_and_fetch(&total, 1);
    return true;
}
template<typename T>
bool RWQueue<T>::Remove()
{
    add_and_fetch(&removed, 1);
    if (!IsExceeded())
    {
        return true;
    }
    mutex.WLock();
    if (!IsExceeded())
    {
        mutex.WUnlock();
        return true;
    }
    RWNode<T>* it = header;
    RWNode<T>* prev = nullptr;
    int count = 0;
    while (it && it != tail)
    {
        RWNode<T>* rem = nullptr;
        if (ALLOW_CLEAR(it->occupied))
        {
            if (it == header)
            {
                header = it->next;
            }
            if (prev)
            {
                prev->next = it->next;
            }
            rem = it;
            count += 1;
        }
        else
        {
            prev = it;
        }
        it = it->next;
        if (rem)
        {
            delete rem;
        }
    }
    if (count > 0)
    {
        sub_and_fetch(&removed, count);
        sub_and_fetch(&total, count);
    }
    mutex.WUnlock();
    return true;
}
template<typename T>
bool RWQueue<T>::IsExceeded()
{
    return removed > SHRINK_MIN &&
           total > 0 && (float)removed / (float)total > SHRINK_THR;
}
template<typename T>
bool RWQueue<T>::Occupy(RWNode<T> *node, int count)
{
    if (!node || count == 0)
    {
        return false;
    }
    int local = node->occupied;
    int expect;
    do {
        if (count > 0 && IS_REMOVE(local))
        {
            return false;
        }
        expect = OCCUPY(local, count);
    } while (!compare_and_set(&(node->occupied), &local, &expect));
    return true;
}
template<typename T>
bool RWQueue<T>::Remove(RWNode<T> *node)
{
    if (!node)
    {
        return false;
    }
    bool suc = true;
    int local = node->occupied;
    int expect;
    do {
        if (IS_REMOVE(local))
        {
            suc = false;
            break;
        }
        expect = FORCE_REMOVE(local);
    } while (!compare_and_set(&(node->occupied), &local, &expect));
    return suc;
}
template <typename T>
class QIterator{
public:
    RWNode<T>* it;
    RWQueue<T>* queue;
    T empty;
    QIterator(RWQueue<T>* queue): it(nullptr), queue(queue){};
    virtual bool HasNext();
    virtual T Next();
    virtual bool Remove();
};
template<typename T>
bool QIterator<T>::HasNext()
{
    int type = queue->mutex.RLock();
    bool has = (it == nullptr && queue->header) || (it && it->next);
    queue->mutex.RUnlock(type);
    return has;
}
template<typename T>
T QIterator<T>::Next()
{
    if (!HasNext())
    {
        return empty;
    }
    T obj = empty;
    int type = queue->mutex.RLock();
    RWNode<T>* next = it == nullptr ? queue->header : it->next;
    while (next && !queue->Occupy(next, 1))
    {
        next = next->next;
    }
    if (next)
    {
        queue->Occupy(it, -1);
        obj = next->t;
        it = next;
    }
    queue->mutex.RUnlock(type);
    return obj;
}
template<typename T>
bool QIterator<T>::Remove()
{
    int type = queue->mutex.RLock();
    bool suc = queue->Remove(it);
    queue->mutex.RUnlock(type);
    queue->Remove();
    return suc;
}
#endif //ANDROID_TQUEUE_H