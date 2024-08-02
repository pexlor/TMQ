#ifndef ANDROID_TMQLIST_H
#define ANDROID_TMQLIST_H

#include <cstring>
#include "Defines.h"

TMQ_NAMESPACE

#define MAX_NAME_LENGTH 128
#define DEFAULT_CAPACITY 16

template <typename T>
class TMQList
{
private:
    char name[MAX_NAME_LENGTH];
    unsigned long capacity;
    unsigned long size;
    T empty;
    T **data;

public:
    TMQList(int capacity = DEFAULT_CAPACITY);
    TMQList(const TMQList &tmqList);
    ~TMQList();
    void SetName(const char *name);
    const char *GetName();
    unsigned long Size();
    T &Get(long index);
    bool Remove(long index);
    bool Remove(T &t);
    bool Add(const T &t);
    void Clear();
};

template <typename T>
TMQList<T>::TMQList(int capacity) : name{0}, capacity(capacity), size(0), data(nullptr)
{
    memset(name, 0, sizeof(name));
    if (capacity <= 0)
    {
        capacity = DEFAULT_CAPACITY;
    }
    data = new T *[capacity];
}

template <typename T>
TMQList<T>::~TMQList()
{
    Clear();
    delete[] data;
    data = nullptr;
    size = 0;
    capacity = DEFAULT_CAPACITY;
}

template <typename T>
bool TMQList<T>::Add(const T &t)
{
    if (data == nullptr || capacity == 0)
    {
        return false;
    }
    if (size >= capacity)
    {
        capacity = capacity + DEFAULT_CAPACITY;
        T **temp = new T *[capacity];
        if (temp == nullptr)
        {
            return false;
        }
        memcpy(temp, data, size * sizeof(T *));
        delete[] data;
        data = temp;
    }
    data[size++] = new T(t);
    return true;
}

template <typename T>
T &TMQList<T>::Get(long index)
{
    if (index < 0 || index >= size)
    {
        return empty;
    }
    return *data[index];
}

template <typename T>
bool TMQList<T>::Remove(long index)
{
    if (index < 0 || index >= size)
    {
        return false;
    }
    delete data[index];
    data[index] = nullptr;
    while (++index < size)
    {
        data[index - 1] = data[index];
        data[index] = nullptr;
    }
    size -= 1;
    return true;
}

template <typename T>
bool TMQList<T>::Remove(T &t)
{
    while (true)
    {
        long index = 0;
        for (; index < size; ++index)
        {
            if (*data[index] == t)
            {
                Remove(index);
                break;
            }
        }
        if (size == 0 || index >= size - 1)
        {
            break;
        }
    }
    return false;
}

template <typename T>
void TMQList<T>::Clear()
{
    for (int i = 0; i < size; ++i)
    {
        delete data[i];
        data[i] = nullptr;
    }
    size = 0;
}

template <typename T>
void TMQList<T>::SetName(const char *name)
{
    memset(this->name, 0, sizeof(this->name));
#ifdef _WIN32
    strncpy_s(this->name, name, MAX_NAME_LENGTH);
#else
    strncpy(this->name, name, MAX_NAME_LENGTH);
#endif // _WIN32
}

template <typename T>
const char *TMQList<T>::GetName()
{
    return name;
}

template <typename T>
unsigned long TMQList<T>::Size()
{
    if (data == nullptr)
    {
        return 0;
    }
    return size;
}

template <typename T>
TMQList<T>::TMQList(const TMQList &tmqList) : size(0), capacity(DEFAULT_CAPACITY), data(nullptr)
{
    memcpy(name, tmqList.name, MAX_NAME_LENGTH);
    capacity = tmqList.capacity;
    if (capacity <= 0)
    {
        capacity = DEFAULT_CAPACITY;
    }
    data = new T *[capacity];
    if (data == nullptr)
    {
        return;
    }
    memset(data, 0, capacity * sizeof(T *));
    // 深拷贝
    for (int i = 0; i < tmqList.size; ++i)
    {
        data[size] = new T(*tmqList.data[i]);
        if (data[size] != nullptr)
        {
            size++;
        }
    }
}

TMQ_NAMESPACE_END

#endif // ANDROID_TMQLIST_H