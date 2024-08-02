//
//  List.h
//  List
//
//  Created by  on 2022/7/15.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef LIST_H
#define LIST_H

#include "string.h"
#include "stdlib.h"
#include "Defines.h"
/// Const definitions for list
// The default capacity for this list.
#define DEFAULT_CAPACITY 16
// Factor for expanding and shrinking the capacity of the list.
#define CAPACITY_FACTOR 0.8
// Invalid index for the list.
#define INVALID_INDEX -1

template<typename T>
class List {
private:
    // The size of the list, the real count of elements.
    TMQSize size;
    // The capacity of the list, will be bigger than the size always.
    TMQSize capacity;
    // A pointer to the elements array.
    T *data;
    // Empty value for special use.
    T empty;
private:
    /**
     * Expand the capacity for this list.
     * @return a boolean value indicate whether the expanding is success or not.
     */
    bool Expand();

    /**
     * Shrink the capacity for this list.
     * @return a boolean value indicate whether the shrinking is success or not.
     */
    bool Shrink();

public:
    /**
     * Construct a list with default capacity.
     * @param capacity, capacity for the list, default value is DEFAULT_CAPACITY.
     */
    List(TMQSize capacity = DEFAULT_CAPACITY);

    /**
     * Construct with an exist list.
     * @param list, original list
     */
    List(const List<T> &list);

    /**
     * Destructor for the list.
     */
    ~List();

    /**
     * Get method for the size.
     * @return the size of the list.
     */
    TMQSize Size();

    /**
     * Checking whether the list is empty or not.
     * @return, true if the list is empty, otherwise false will be returned.
     */
    bool Empty();

    /**
     * Get method for the element at position index.
     * @param index, the index to find.
     * @return, the reference to the element. If there is no element at index, empty object will be
     *  returned.
     */
    T &Get(int index);

    /**
     * Set a new value at index.
     * @param index, the position to set.
     * @param t, the new value.
     */
    void Set(int index, const T &t);

    /**
     * Remove the element at index.
     * @param index, the position to remove.
     * @return a boolean value indicate the remove is success or not.
     */
    bool Remove(int index);

    /**
     * Batch for removing elements. This operation will remove count elements at start index at once.
     * @param start, start index to remove
     * @param count, total count to be removed.
     * @return a boolean value indicate the remove is success or not.
     */
    bool Remove(int start, int count);

    /**
     * Add a element to the end of the list.
     * @param t, the element to add.
     * @return int, the index of this value.
     */
    int Add(const T &t);

    /**
     * Insert a element to the index position. The elements after index will be moved to next one
     * by one.
     * @param index, index to insert.
     * @param t, a new value to insert.
     * @return a boolean value indicate the remove is success or not.
     */
    bool Insert(int index, const T &t);

    /**
     * Clear the list. The size will be set to zero after clear.
     */
    void Clear();
};

/*
 * Implementation of the default constructor. The min value of the capacity is set to
 * DEFAULT_CAPACITY. The memory under capacity will be applied in advance.
 */
template<typename T>
List<T>::List(TMQSize capacity)
        :capacity(capacity), size(0), data(nullptr) {
    if (capacity <= DEFAULT_CAPACITY) {
        capacity = DEFAULT_CAPACITY;
    }
    data = (T *) calloc(capacity, sizeof(T));
}

/*
 * Construct a list using an exist list.
 */
template<typename T>
List<T>::List(const List<T> &list) :capacity(list.capacity), size(list.size), data(nullptr) {
    if (capacity < DEFAULT_CAPACITY) {
        capacity = DEFAULT_CAPACITY;
    }
    data = (T *) calloc(capacity, sizeof(T));
    if (data) {
        // Copy all elements from original list.
        for (int i = 0; i < size; ++i) {
            data[i] = list.data[i];
        }
    }
}

/*
 * Upon destructing, clear(release) all element first.
 */
template<typename T>
List<T>::~List() {
    Clear();
    // free the memory.
    if (data) {
        free(data);
    }
    size = 0;
    capacity = DEFAULT_CAPACITY;
}

// Return the size directly.
template<typename T>
TMQSize List<T>::Size() {
    return size;
}

// Get element at index.
template<typename T>
T &List<T>::Get(int index) {
    // Check whether index is valid or not.
    return index >= 0 && index < size ? data[index] : empty;
}

/*
 * Remove element at index. Three key steps:
 * 1. Destruct the element at index.
 * 2. Move the elements to forward one by one.
 * 3. Toggle the shrink() to reduce memory.
 */
template<typename T>
bool List<T>::Remove(int index) {
    if (index < 0 || !data) {
        return false;
    }
    // Destruct the element at index.
    data[index].~T();
    // Move the elements to forward one by one.
    for (int i = index; i < size && i + 1 < size; ++i) {
        memcpy(&data[i], &data[i + 1], sizeof(T));
    }
    memset(&data[size - 1], 0, sizeof(T));
    // Reduce the size by one.
    size = size - 1;
    // Toggle the shrink() to reduce memory.
    Shrink();
    return true;
}

/*
 * Add a element. The new element will be appended to the tail. Before appending, we will toggle the
 * Expand to enlarge the list capacity.
 */
template<typename T>
int List<T>::Add(const T &t) {
    // Expand the list first, if data is nullptr or expand fail, INVALID_INDEX will be returned.
    if (!data || !Expand()) {
        return INVALID_INDEX;
    }
    data[size++] = t;
    return size - 1;
}

/*
 * Clear elements in list. Two important steps:
 * 1. Destruct all elements
 * 2. Check whether the capacity is larger than DEFAULT_CAPACITY,
 *  if true, shrink to DEFAULT_CAPACITY
 */
template<typename T>
void List<T>::Clear() {
    // Destruct all elements
    for (int i = 0; i < size; ++i) {
        data[i].~T();
    }
    size = 0;
    // Reset capacity.
    if (capacity > DEFAULT_CAPACITY) {
        free(data);
        data = (T *) calloc(capacity, sizeof(T));
    }
}

/*
 * Expand task to enlarge the capacity. The new capacity will be calculate using method below:
 *      capacity = capacity * (2 - CAPACITY_FACTOR);
 * This means that the new capacity is relative to the old capacity.
 */
template<typename T>
bool List<T>::Expand() {
    // no need to expand.
    if (size < capacity) {
        return true;
    }
    // the new capacity calculated by capacity * (2 - CAPACITY_FACTOR)
    int expand = capacity * (2 - CAPACITY_FACTOR);
    T *ds = (T *) (calloc(expand, sizeof(T)));
    if (ds) {
        // Copy data to the new memory space.
        memcpy(ds, data, size * sizeof(T));
        free(data);
        // Assigned the value for data and capacity.
        data = ds;
        capacity = expand;
        return true;
    }
    return false;
}

/*
 * Check empty by size.
 */
template<typename T>
bool List<T>::Empty() {
    return size == 0;
}

/*
 * Shrink the list, the new capacity after shrinking will be calculated by
 *  capacity = capacity * CAPACITY_FACTOR
 */
template<typename T>
bool List<T>::Shrink() {
    int shrink = capacity * CAPACITY_FACTOR;
    if (size <= shrink && size > DEFAULT_CAPACITY) {
        T *ds = (T *) calloc(shrink, sizeof(T));
        if (ds) {
            // Copy all elements
            memcpy(ds, data, size * sizeof(T));
            free(data);
            data = ds;
            capacity = shrink;
        }
    }
    return true;
}

/*
 * Insert a new value at index.
 */
template<typename T>
bool List<T>::Insert(int index, const T &t) {
    // Check the index parameter, and ensure it to be valid.
    // Change the index to header if the index < 0.
    if (index < 0) {
        index = 0;
    }
    // Change the index to tail if the index > size.
    if (index > size) {
        index = size;
    }
    // Expand the list, if fail, return immediately.
    if (!Expand()) {
        return false;
    }
    // Move all elements after index to their next.
    for (int i = size; i > index; --i) {
        memcpy(&data[i], &data[i - 1], sizeof(T));
    }
    // Assigned the new value to the data[index]
    data[index] = t;
    // Add size by 1.
    size = size + 1;
    return true;
}

/*
 * Change the value at index. Before assigning the new value, destruct the old value first.
 */
template<typename T>
void List<T>::Set(int index, const T &t) {
    if (index >= 0 && index < size) {
        data[index].~T();
        data[index] = t;
    }
}

/*
 * Batch remove elements.
 */
template<typename T>
bool List<T>::Remove(int start, int count) {
    if (count <= 0 || start < 0) {
        return false;
    }
    int next = start + count;
    int left = size - next > count ? count : size - next;
    // Loop to move the element after stat + count to the front.
    while (next < size && left > 0) {
        memcpy(&data[start], &data[next], left * sizeof(T));
        next = next + count;
        left = size - next > count ? count : size - next;
    }
    size = size - count;
    if (size < 0) {
        size = 0;
    }
    memset(&data[size], 0, (capacity - size) * sizeof(T));
    // Shrink the list.
    return Shrink();
}


#endif //LIST_H
