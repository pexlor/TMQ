//
//  SectionAllocator.h
//  SectionAllocator
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef SECTION_ALLOCATOR_H
#define SECTION_ALLOCATOR_H

#include "Metas.h"
#include "RbTree.h"
#include "Persistence.h"

/// Const definitions
// Section address definitions. A section address is made up with the index of the the element and
// its allocId. Section address is also a type of TMQAddress, which has the length 64 bits. The high
// 32 bits represents the alloc index, while the low 32 bits represents the allocId.
#define SECTION_INDEX(address)              (unsigned int)(address >> 32)
#define SECTION_ALLOC_ID(address)           (unsigned int)(address)
#define SECTION_ADDRESS(index, allocId)      ((TMQAddress)index << 32 | (TMQAddress)allocId)

/**
 * A section allocator is a partly implementation for the ISectionSpace, using to allocate and
 * deallocate the linear space. The most important capability of the section allocator is to do the
 * address reuse. The second important benefit of the section allocator is that it replaces the
 * remove operation of an element in the lazy linear list with the add operation, which can improve
 * the efficiency very much.
 */
class SectionAllocator : public ISectionSpace {
private:
    // A RbTree to store the freed allocations with an ascending order of the tmq address.
    RbTree<TMQAddress, SecAlloc> freedAllocTree;

protected:
    /**
     * Internal method for address reuse.
     * @param size, the size to reallocate.
     * @return, a reused tmq address.
     */
    TMQAddress ReuseAddress(TMQSize size);

    /**
     * Find a MetaAlloc by a section address.
     * @param secAddress, the section address from method Allocate
     * @param metaAlloc, a MetaAlloc for saving the result.
     * @return the index of the found element in the linear space.
     */
    int FindAlloc(TMQAddress secAddress, SecAlloc &metaAlloc);

    /**
     * Internal method, read the freed allocation and put them into the freedAllocTree. Some time,
     * reading data from the linear space implemented by disk file will be time-consumed. So this
     * method will read data in batches.
     */
    void GetFreedAllocList();

    /**
     * Try to free pages. It will calculate the freed pages from the tmq address. When deallocate a
     * tmq address, we will collect the freed tmq address with the same page. If all of the address
     * fragments have been freed, its associated page will be freed too.
     * @param address, a tmq address to free.
     */
    bool TryFreePage(TMQAddress address);

    /**
     * Get method for the lazy linear list for element type SecAlloc.
     * @param reserve, reserve count for this require.
     * @return, a pointer to the LazyLinearList.
     */
    virtual LazyLinearList<SecAlloc> *GetLazyAllocList(TMQSize reserve = 0) = 0;

    /**
     * A virtual method to allocate pages.
     * @param size, required page size.
     * @param allocSize, the real size to be allocated.
     * @return the page index.
     */
    virtual int AllocPages(int size, int *allocSize) = 0;

    /**
     * Get the size of the allocated page.
     * @param page, page index to find.
     * @return, the size of the allocated page.
     */
    virtual int GetAllocPageSize(int page) = 0;

    /**
     * Deallocate the page.
     * @param page, page to deallocate.
     */
    virtual void DeallocPages(int page) = 0;

    /**
     * Release an allocated section address.
     * @param secAddress, the section address to release.
     * @return a boolean value indicate whether the release is success or not.
     */
    virtual bool ReleaseAlloc(TMQAddress secAddress) = 0;

    /**
     * Append a alloc info to the section space.
     * @param tmqAddress, a tmq address from linear space.
     * @param size, the size of the address.
     * @param state, alloc state of this address: ADDRESS_ALLOC, ADDRESS_FREE, ADDRESS_RELEASE
     * @return a tmq address in the section.
     */
    virtual TMQAddress AppendAlloc(TMQAddress tmqAddress, TMQSize size, TMQLState state) = 0;

public:
    /**
     * Allocate the linear storage space with the specified length.
     * @param length, the length of the linear storage space to allocate.
     * @return, a tmq address, ADDRESS_NULL will be return if allocate fail.
     */
    virtual TMQAddress Allocate(TMQLSize length);

    /**
     * Deallocate a tmq address.
     * @param address, the address to deallocate.
     */
    virtual void Deallocate(TMQAddress address);
};


#endif //SECTION_ALLOCATOR_H
