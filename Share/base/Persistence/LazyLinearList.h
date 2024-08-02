//
//  LazyLinearList.h
//  LazyLinearList
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef LAZY_LINEAR_LIST_H
#define LAZY_LINEAR_LIST_H

#include "Defines.h"
#include "LinearSpace.h"
#include "PageSpace.h"

// type define for the comparison between *p1 and *p2
typedef int (*Compare)(void *p1, void *p2);

/**
 * Template class for a list that can read and write on linear storage space. It is lazy read-write,
 *  except the size and capacity, all the data is saved on linear space. The format of a lazy linear
 *  list describes below:
 *      [size(TMQSize)][T array...]:
 *  size describes the length of template object array, after the size, it is all elements stored
 *  one after another. The lazy linear list can not expand or shrink its capacity. One should expand
 *  or shrink the capacity by oneself. When construct a LazyLinearList, its capacity is determined
 *  by the pages info and size of it element. So we can use lazy linear list as a safe reader but be
 *  careful with its adding.
 *
 * @tparam T, the template name.
 */
template<typename T>
class LazyLinearList {
protected:
    // A pointer th the page space.
    IPageSpace *linearSpace;
    // Start tmq address of the lazy linear list.
    TMQAddress address;
    // Capacity of this list.
    TMQLSize capacity;
    // Elements count in this list.
    TMQLSize size;

public:
    /**
     * Default constructor.
     */
    LazyLinearList() : linearSpace(nullptr), address(ADDRESS_NULL), capacity(0), size(0) {

    }

    /**
     * Construct a LazyLinearList with linearSpace, start address and capacity.
     * @param linearSpace, a pointer to the page space.
     * @param address, start address of the list.
     * @param capacity, total capacity of this list.
     */
    LazyLinearList(IPageSpace *linearSpace, TMQAddress address, TMQLSize capacity);

    /*
     * Default destructor.
     */
    ~LazyLinearList();

    /**
     * Get method for the elements count.
     * @return TMQSize, the count of the elements.
     */
    TMQLSize GetSize();

    /**
     * Get method for the capacity.
     * @return the capacity.
     */
    TMQLSize GetCapacity();

    /**
     * Set method to modify the size.
     * @param size, the size to set.
     */
    void SetSize(TMQLSize size);

    /**
     * Get method for the element at index.
     * @param index, the index of the element
     * @return T, the element at index.
     */
    T Get(long index);

    /**
     * Remove the element at index. After remove, the elements after index will be moved to their
     * previous position one by one.
     * @param index, the index to remove.
     * @return bool, a boolean value indicate whether the remove is success or not.
     */
    bool Remove(long index);

    /**
     * Add a element with compare. If the compare is nullptr, it will append the element at the last
     * of the list, and set size = size + 1.
     * @param t, the value of the element.
     * @param compare, a Compare to find a correct position to set the value.
     * @return int, a position for this value.
     */
    long Add(const T &t, Compare compare = nullptr);

    /**
     * Find a position for t with compare.
     * @param t, the value to find.
     * @param compare, a method to compare elements.
     * @return int, a int value indicate the index of the element, will be [0-size].
     */
    long FindPosition(const T &t, Compare compare = nullptr);

    /**
     * Insert a value a index. The elements after index will be moved to their next position.
     * @param index, the index to insert.
     * @param t, the value.
     * @return bool, a boolean value indicate whether the insert is success or not.
     */
    bool Insert(long index, const T &t);

    /**
     * Clear the elements, the size will be zero after clear.
     */
    void Clear();

    /**
     * Set method to modify element at index.
     * @param index, index to set.
     * @param t, the new value.
     */
    void Set(long index, const T &t);

    /**
     * Inline method to calculate the page offset for index of the list.
     * @param index, index on the list.
     * @return, a page offset.
     */
    inline long Offset(long index) {
        if (index >= 0 && index <= size) {
            return OFFSET(address) + 2 * sizeof(TMQSize) + index * sizeof(T);
        }
        return -1;
    }

    /**
     * A inline method to check the list is empty or not.
     * @return a boolean value indicate whether the list is empty or not.
     */
    inline bool Empty() {
        return size == 0;
    }
};

/*
 * Construct the LazyLinearList. The initial size will be read from address in linearSpace. And the
 * capacity can be calculated by the length of this page space and the size of T.
 */
template<typename T>
LazyLinearList<T>::LazyLinearList(IPageSpace *linearSpace, TMQAddress address, TMQLSize length)
        :address(0), size(0), capacity(0) {
    this->linearSpace = linearSpace;
    this->address = address;
    int page = PAGE(address);
    int offset = OFFSET(address);
    // Read value of the size.
    linearSpace->Read(page, offset, &size, sizeof(TMQLSize));
    this->capacity = (length - 2 * sizeof(TMQLSize)) / sizeof(T);
}

/*
 * Constructor.
 */
template<typename T>
LazyLinearList<T>::~LazyLinearList() {
    size = 0;
    capacity = 0;
}

/*
 * Return the size directly.
 */
template<typename T>
TMQLSize LazyLinearList<T>::GetSize() {
    return size;
}

/*
 * SetSize will change the size variable in memory and save the value to the page space at the same
 * time.
 */
template<typename T>
void LazyLinearList<T>::SetSize(TMQLSize newSize) {
    linearSpace->Write(PAGE(address), OFFSET(address), &newSize, sizeof(TMQSize));
    size = newSize;
}

/*
 * Get the element at index. It will invoke the read method of the page space for the data.
 * If the index is invalid, an empty value 'obj' will be returned.
 */
template<typename T>
T LazyLinearList<T>::Get(long index) {
    T obj;
    if (index >= 0 && index < GetSize()) {
        // Get value from the linear space.
        linearSpace->Read(PAGE(address), Offset(index), &obj, sizeof(T));
    }
    return obj;
}

/*
 * Remove a element, the elements after this element will be moved to their previous position.
 * So when there are many elements to move, it will be time consumed.
 */
template<typename T>
bool LazyLinearList<T>::Remove(long index) {
    if (index < 0 || index > GetSize() - 1) {
        return false;
    }
    // Move the elements after index.
    for (int i = index; i + 1 < size; ++i) {
        T tmp;
        linearSpace->Read(PAGE(address), Offset(i + 1), &tmp, sizeof(T));
        linearSpace->Write(PAGE(address), Offset(i), &tmp, sizeof(T));
    }
    // Modify the size to size - 1.
    SetSize(GetSize() - 1);
    return true;
}

/**
 * Insert an element into index. the elements after this element will be moved to their next
 * position. So when there are many elements to move, it will be time consumed.
 * @tparam T, template name.
 * @param index, the index to insert.
 * @param t, the value.
 * @return, bool, a boolean value indicate whether the insert is success or not.
 */
template<typename T>
bool LazyLinearList<T>::Insert(long index, const T &t) {
    // Check and ensure the index is valid.
    if (index < 0) {
        index = 0;
    }
    // If the index >= GetSize, which means append the element to the tail of the list. So invoke
    // Add to insert this element directly.
    if (index >= GetSize()) {
        return Add(t) >= 0;
    }
    // Overflow occurs, ignore the insert and return false.
    if (GetSize() + 1 > capacity) {
        return false;
    }
    // Move the elements to their next position.
    for (int i = GetSize(); i > index; --i) {
        T tmp;
        linearSpace->Read(PAGE(address), Offset(i - 1), &tmp, sizeof(T));
        linearSpace->Write(PAGE(address), Offset(i), &tmp, sizeof(T));
    }
    // Write the t to the position of index.
    linearSpace->Write(PAGE(address), Offset(index), (void *) &t, sizeof(T));
    // Modify the size to size + 1.
    SetSize(GetSize() + 1);
    return true;
}

/**
 * It is unnecessary to deal with every elements, set the size to zero directly.
 * @tparam T
 */
template<typename T>
void LazyLinearList<T>::Clear() {
    SetSize(0);
}

/*
 * Set a new value to index.
 */
template<typename T>
void LazyLinearList<T>::Set(long index, const T &t) {
    // Check parameters.
    if (index < 0 || index > capacity) {
        return;
    }
    linearSpace->Write(PAGE(address), Offset(index), (void *) &t, sizeof(T));
}

/*
 * Add element with a compare function. If there is no compare, add the element to the tail of the
 * list. Otherwise, find the position for this element using FindPosition, and then insert the value
 * to its position.
 */
template<typename T>
long LazyLinearList<T>::Add(const T &t, Compare compare) {
    if (GetSize() + 1 > capacity) {
        return -1;
    }
    // Add to the end of the list.
    if (compare == nullptr) {
        linearSpace->Write(PAGE(address), Offset(GetSize()), (void *) &t, sizeof(T));
        SetSize(GetSize() + 1);
        return GetSize() - 1;
    }
    // Find the position for this element.
    int pos = FindPosition(t, compare);
    // Insert the value to its position and return the final position.
    if (Insert(pos, t)) {
        return pos;
    }
    return -1;
}

template<typename T>
TMQLSize LazyLinearList<T>::GetCapacity() {
    return capacity;
}

/*
 * Quick sort algorithm to find the position of the value.
 */
template<typename T>
long LazyLinearList<T>::FindPosition(const T &t, Compare compare) {
    if (compare == nullptr) {
        return GetSize();
    }
    long start = 0, end = GetSize() - 1, p = -1;
    int cmp;
    T obj;
    while (start <= end) {
        p = (start + end) / 2;
        linearSpace->Read(PAGE(address), Offset(p), &obj, sizeof(T));
        // Compare the two elements.
        cmp = compare((void *) &t, (void *) &obj);
        if (cmp > 0) {
            start = p + 1;
            p = start;
        } else if (cmp < 0) {
            end = p - 1;
        } else {
            break;
        }
    }
    // Return the final position p.
    return p;
}


#endif //LAZY_LINEAR_LIST_H
