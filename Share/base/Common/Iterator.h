//
//  Iterator.h
//  Iterator
//
//  Created by  on 2022/3/15.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef ITERATOR_H
#define ITERATOR_H

/*
 * A template class for iterator. In this interface, we define the base operation of a iterator,
 * such as HasNext, Next, Remove.
 */
template<typename T>
class Iterator {
public:
    /**
     * Check whether the iterator has next or not.
     * @return a boolean value, true if it has next, otherwise false.
     */
    virtual bool HasNext() = 0;

    /**
     * Achieve the next element and more the iterator to the next at the same time.
     * @return
     */
    virtual T Next() = 0;

    /**
     * Safe to remove the element.
     * @return a boolean value indicate whether the remove action is success or not.
     */
    virtual bool Remove() = 0;

    /**
     * Destruct for this virtual class.
     */
    virtual ~Iterator() {}
};

#endif //ITERATOR_H
