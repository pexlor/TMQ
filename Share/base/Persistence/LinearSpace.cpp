//
//  LinearSpace.h
//  LinearSpace
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef LINEAR_SPACE_H
#define LINEAR_SPACE_H

#include "Metas.h"
#include "Iterator.h"
#include "List.h"

/**
 * ILinearSpace is the interface for linear space, it works on the page space, but divides them into
 * smaller units using for the storage allocation. It is the management of linear storage space,
 * which can allocate and deallocate storage space with specified length. Besides, It also provides
 * APIs to read/write data from/to the linear space with a tmq address allocated by the linear space.
 */
class ILinearSpace {
public:
    /**
     * Allocate a linear storage space with specified length.
     * @param length, the length required to allocate.
     * @return a tmq address describe the allocated linear space.
     */
    virtual TMQAddress Allocate(TMQLSize length) = 0;

    /**
     * Deallocate a tmq address that is allocated by Allocate().
     * @param address, a tmq address to deallocate.
     */
    virtual void Deallocate(TMQAddress address) = 0;

    /**
     * Read data from address and save the data to the result buffer. If success, the read length
     * will be return, otherwise, -1 will be return.
     * @param address, address to read.
     * @param buf, a pointer to the buffer to save the results.
     * @param length, length to read.
     * @return long, a long value that has read.
     */
    virtual TMQLSize Read(TMQAddress address, void *buf, TMQLSize length) = 0;

    /**
     * Write data into a tmq address.
     * @param address, the address to write to.
     * @param data, a pointer to the data.
     * @param length, the length of the data.
     * @return, long, the written length.
     */
    virtual TMQLSize Write(TMQAddress address, void *data, TMQLSize length) = 0;

    /**
     * Copy data for source address to the destination address in the linear space.
     * @param dst, destination address to copy to.
     * @param src, source address to copy from.
     * @param length, the length to copy.
     */
    virtual void Copy(TMQAddress dst, TMQAddress src, TMQLSize length) = 0;

    /**
     * Zero a tmq address with specified length.
     * @param address, a tmq address
     * @param length, the length to set zero.
     */
    virtual void Zero(TMQAddress address, TMQLSize length) = 0;

    /**
     * Virtual destructor.
     */
    virtual ~ILinearSpace() {};
};

/**
 * ISectionSpace is inherited from ILinearSpace, but divides the total linear space into multiply
 * named section space. It is the functional layer using to save the objects that has the similar
 * attributes.
 */
class ISectionSpace : public ILinearSpace {
public:
    /**
     * Get method for the section name.
     * @return, a count pointer to the section name.
     */
    virtual const char *GetName() = 0;

    /**
     * Get the tmq address list that has allocated.
     * @param allocList, a list for saving results.
     * @return, bool, a boolean value indicate whether there are allocations or not.
     */
    virtual bool GetAllocList(List<MetaAlloc> &allocList) = 0;

    /**
     * Virtual destructor.
     */
    virtual ~ISectionSpace() {};
};


#endif //LINEAR_SPACE_H
