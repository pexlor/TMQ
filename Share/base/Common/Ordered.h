//
//  Ordered.h
//  Ordered
//
//  Created by  on 2022/4/12.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef ORDERED_H
#define ORDERED_H

#include "List.h"

/**
 * A quick compare function defined to compare between two elements. the return value should be:
 *  1. *p1 > *p2, return a integer value above 0.
 *  2. *p1 == *p2, return zero value.
 *  3. *p1 < *p2, return a integer value below 0.
 */
typedef int (*QuickCompare)(void *p1, void *p2);

/*
 * The class Ordered<T> inherited from List<T> is used to add elements as ordered. And the
 * Ordered<T> provides an ability to find a element quickly. The elements in the Ordered are all
 * sorted. It uses quick sort algorithm to insert and search element. So the Ordered list is more
 * effective on searching(find a element), But not effective on modifying such as insert or remove.
 */
template<typename T>
class Ordered : public List<T> {
public:
    /**
     * Add an element using QuickCompare. This function will find the position of the element first,
     * The insert the element by invoking insert method of its base class.
     * @param t, the reference to the value to be add.
     * @param compare, a function pointer to QuickCompare.
     * @return, the position of this element.
     */
    int Add(const T &t, QuickCompare compare);

    /**
     * Find the position of t. If it is not exist, the function will return a position
     * that this value can insert into.
     * @param t, the value.
     * @param compare, a function pointer to QuickCompare.
     * @return the position of the value or a position that this value can be set to.
     */
    int FindPosition(const T &t, QuickCompare compare);

    /**
     * Get the real position of the value t. Unlike the FindPosition method, it will return
     * INVALID_INDEX if the value is not exists.
     * @param t, the value to find.
     * @param compare, a function pointer to QuickCompare.
     * @return the real position of the value t, INVALID_INDEX will be returned if it is not exist.
     */
    int GetPosition(const T &t, QuickCompare compare);

    /**
     * Check whether the element is exist or not. It is same with GetPosition, but the return value
     * type is boolean.
     * @param t, the value to find.
     * @param compare, a function pointer to QuickCompare.
     * @return, true if it is exist, otherwise false will be returned.
     */
    bool Exist(const T &t, QuickCompare compare);
};

/*
 * Quick sort algorithm to find a element.
 */
template<typename T>
int Ordered<T>::FindPosition(const T &t, QuickCompare compare) {
    // if there is no compare or the list is empty, return the tail of the list.
    if (compare == nullptr || List<T>::Empty()) {
        return List<T>::Size();
    }
    // Quick sort algorithm to find the position.
    long start = 0, end = List<T>::Size() - 1, p = -1;
    int cmp;
    T obj;
    while (start <= end) {
        p = (start + end) / 2;
        cmp = compare((void *) &t, (void *) &List<T>::Get(p));
        if (cmp > 0) {
            start = p + 1;
            p = start;
        } else if (cmp < 0) {
            end = p - 1;
        } else {
            break;
        }
    }
    return p;
}

/*
 * Add a new element. Two key steps:
 * 1. Find a suit position of the new element by invoking FindPosition.
 * 2. Insert the value to that position.
 */
template<typename T>
int Ordered<T>::Add(const T &t, QuickCompare compare) {
    // Find the position for this element.
    int position = FindPosition(t, compare);
    if (position < 0) {
        position = 0;
    }
    if (position > List<T>::Size()) {
        position = List<T>::Size();
    }
    // Insert the element to position.
    List<T>::Insert(position, t);
    return position;
}

/*
 * Get the real position. This method will find the element position at first, and then it will
 * compare the element at pos to check if the element at position is equal to current value or not.
 */
template<typename T>
int Ordered<T>::GetPosition(const T &t, QuickCompare compare) {
    int pos = FindPosition(t, compare);
    if (pos >= 0 && pos < List<T>::Size()) {
        T &pt = List<T>::Get(pos);
        // Compare to make sure that the element at pos is equal to what we are finding.
        if (compare((void *) &t, (void *) &List<T>::Get(pos)) == 0) {
            return pos;
        }
    }
    return -1;
}

/*
 * Simple method for checking a value is exist or not.
 */
template<typename T>
bool Ordered<T>::Exist(const T &t, QuickCompare compare) {
    int pos = GetPosition(t, compare);
    return pos >= 0 && pos < List<T>::Size();
}

#endif //ORDERED_H
