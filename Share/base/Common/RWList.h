//
// Created by 大老鼠 on 2023/1/28.
//

#ifndef RW_LIST_H
#define RW_LIST_H

#include "LinkList.h"
#include "RWMutex.h"

template<typename T>
class RWList{
public:
    Node<T>* header;
    Node<T>* tail;
    Node<T> empty;
    RWMutex mutex;

    void Enqueue(const T& t)
    {

    }
    T Consume(ValueCompare* compare)
    {
        T value;
        int type = mutex.RLock();
        if(header == nullptr)
        {
            value = empty.value;
        }
        mutex.RUnlock(type);
        return value;
    }
};

#endif //RW_LIST_H
