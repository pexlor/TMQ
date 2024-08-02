#ifndef ANDROID_LIST_H
#define ANDROID_LIST_H

#include <malloc.h>
#include "cstring"

#include "Defines.h"

#define DEFAULT_CAPACITY 16
#define CAPACITY_FACTOR 0.8
#define INVALID_INDEX -1

TMQ_NAMESPACE

template <typename T>
class List
{
private:
    unsigned int size;
    unsigned int capacity;
    T *data;
    T empty;

private:
    bool Expand();
    bool Shrink();

public:
    List(unsigned int capacity = DEFAULT_CAPACITY);
    List(const List<T> &list);
    ~List();
    unsigned int Size();
    bool Empty();
    T &Get(int index);
    void Set(int index, const T &t);
    bool Remove(int index);
    bool Remove(int start, int count);
    int Add(const T &t);
    bool Insert(int index, const T &t);
    void Clear();
};

template <typename T>
List<T>::List(unsigned int capacity)
    : capacity(capacity), size(0), data(nullptr)
{
    if (capacity <= DEFAULT_CAPACITY)
    {
        capacity = DEFAULT_CAPACITY;
    }
    data = (T *)calloc(capacity, sizeof(T));
}

template <typename T>
List<T>::List(const List<T> &list) : capacity(list.capacity), size(list.size), data(nullptr)
{
    if (capacity < DEFAULT_CAPACITY)
    {
        capacity = DEFAULT_CAPACITY;
    }
    data = (T *)calloc(capacity, sizeof(T));
    if (data)
    {
        for (int i = 0; i < size; ++i)
        {
            data[i] = list.data[i];
        }
    }
}

template <typename T>
List<T>::~List()
{
    if (data && size > 0)
    {
        free(data);
    }
}

template <typename T>
unsigned int List<T>::Size()
{
    return size;
}

template <typename T>
T &List<T>::Get(int index)
{
    return index >= 0 && index < size ? data[index] : empty;
}

template <typename T>
bool List<T>::Remove(int index)
{
    if (index < 0 || !data)
    {
        return false;
    }
    data[index].~T();
    for (int i = index; i < size && i + 1 < size; ++i)
    {
        memcpy(&data[i], &data[i + 1], sizeof(T));
    }
    memset(&data[size - 1], 0, sizeof(T));
    size = size - 1;
    Shrink();
    return true;
}

template <typename T>
int List<T>::Add(const T &t)
{
    if (!data || !Expand())
    {
        return INVALID_INDEX;
    }
    data[size++] = t;
    return size - 1;
}

template <typename T>
void List<T>::Clear()
{
    if (data)
    {
        for (int i = 0; i < size; ++i)
        {
            data[i].~T();
        }
        free(data);
    }
}

template <typename T>
bool List<T>::Expand()
{
    if (size < capacity)
    {
        return true;
    }
    int expand = capacity * (2 - CAPACITY_FACTOR);
    T *ds = (T *)(calloc(expand, sizeof(T)));
    if (!ds)
    {
        return false;
    }
    memcpy(ds, data, size * sizeof(T));
    free(data);
    data = ds;
    capacity = expand;
    return true;
}

template <typename T>
bool List<T>::Empty()
{
    return size == 0;
}

template <typename T>
bool List<T>::Shrink()
{
    int shrink = capacity * CAPACITY_FACTOR;
    if (size <= shrink && size > DEFAULT_CAPACITY)
    {
        T *ds = (T *)calloc(shrink, sizeof(T));
        if (ds)
        {
            memcpy(ds, data, size * sizeof(T));
            free(data);
            data = ds;
            capacity = shrink;
        }
    }
    return true;
}

template <typename T>
bool List<T>::Insert(int index, const T &t)
{
    if (index < 0)
    {
        index = 0;
    }
    if (index > size)
    {
        index = size;
    }
    if (!Expand())
    {
        return false;
    }
    for (int i = size; i > index; --i)
    {
        memcpy(&data[i], &data[i - 1], sizeof(T));
    }
    data[index] = t;
    size = size + 1;
    return true;
}

template <typename T>
void List<T>::Set(int index, const T &t)
{
    if (index >= 0 && index < size)
    {
        data[index] = t;
    }
}

template <typename T>
bool List<T>::Remove(int start, int count)
{
    if (count <= 0 || start < 0)
    {
        return false;
    }
    int next = start + count;
    int left = size - next > count ? count : size - next;
    while (next < size && left > 0)
    {
        memcpy(&data[start], &data[next], left * sizeof(T));
        next = next + count;
        left = size - next > count ? count : size - next;
    }
    size = size - count;
    if (size < 0)
    {
        size = 0;
    }
    memset(&data[size], 0, (capacity - size) * sizeof(T));
    return Shrink();
}

TMQ_NAMESPACE_END

#endif // ANDROID_LIST_H