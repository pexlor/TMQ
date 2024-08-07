//
//  SectionSpace.h
//  SectionSpace
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef SECTION_SPACE_H
#define SECTION_SPACE_H

#include "Metas.h"
#include "LinearSpace.h"
#include "Persistence.h"
#include "LazyLinkList.h"
#include "Ordered.h"
#include "SectionAllocator.h"

/// Const definitions
// Inner section name for allocated pages.
#define SECTION_ALLOC                           "__ALLOC__"
// Max allocated id for int.
#define ALLOC_ID_MAX                            0xffffffff
#define SPACE_RESET_THRESHOLD                   0.3
// Element reserved count for lazy linear list.
#define LAZY_RESERVE_READ                       0
#define LAZY_RESERVE_WRITE                      1

/**
 * SectionSpace is inherited from SectionAllocator. With this class, we implement all of the
 * APIs defined in ISectionSpace, such as Read, Write, Copy, Zero and other virtual function
 * inherited from iis parent. Besides, SectionSpace holds the name, and all of the SecAlloc. we use
 * a LazyLinearList to manage all of the SecAlloc, that we must be careful with the space expand and
 * shrink. In order to ensure the section space allocation efficiency, it is not timely for cleaning
 * the section space. The const value SPACE_RESET_THRESHOLD is used to toggle the ResetSpace task.
 */
class SectionSpace : public SectionAllocator {
private:
    // Array for saving the section name.
    char name[SECTION_NAME_LEN];
    // A pointer to the persistence
    Persistence *persist;
    // Alloc id for section allocation.
    unsigned int allocId = 0;
    // The count of release.
    unsigned int releaseCount;
    // Meta section describes the position of the section space.
    MetaSection metaSection;
    // Section allocation list info.
    LazyLinearList<SecAlloc> lazyAllocList;

public:
    /**
     * Construct a Section Space with persistence and a pointer to the name.
     * @param persist, a pointer to the persistence.
     * @param sec, a pointer the the section name.
     */
    SectionSpace(Persistence *persist, const char *sec);

    /**
     * Default destruct.
     */
    virtual ~SectionSpace();

    /**
     * Reset and clean the section space. When this task occurs, the lazy linear list will shrink.
     * the SecAlloc with release state will be removed.
     * @return the size of the SecAlloc list.
     */
    unsigned int ResetSpace();

    /**
     * Get method for the section name.
     * @return a const pointer the the section name.
     */
    const char *GetName();

    /**
     * Get method to collect the MetaAlloc list from section space.
     * @param allocList, a reference to MetaAlloc list to save results
     * @return, a boolean value whether it is success or not.
     */
    virtual bool GetAllocList(List<MetaAlloc> &allocList);

    /**
     * Get the pointer to the lazy linear list with the required reserve count. For example, if you
     * only want to do read on this list, reserve count can be zero. Or if you want to add one or
     * more element into the list, the reserve count must be the count you want to add. When the
     * reserved count can not be fulfilled, the section space will enlarge itself.
     * @param reserve, reserve count of the LazyLinearList to ensure
     * @return, a pointer to LazyLinearList.
     */
    virtual LazyLinearList<SecAlloc> *GetLazyAllocList(TMQSize reserve);

    /**
     * Release a SecAlloc, This will only change the state of the SecAlloc associated with this tmq
     * section address, and then toggle a ResetSpace task.
     * @param secAddress, a tmq address to a SecAlloc.
     * @return, bool, a boolean value indicate whether the release is success or not.
     */
    virtual bool ReleaseAlloc(TMQAddress secAddress);

    /**
     * Append a SecAlloc and return section address.
     * @param tmqAddress, a tmq address to MetaAlloc.
     * @param size, the size with this address.
     * @param state, the state of this SectionAlloc, ADDRESS_ALLOC, ADDRESS_FREE.
     * @return, a tmq address in section address.
     */
    virtual TMQAddress AppendAlloc(TMQAddress tmqAddress, TMQSize size, TMQLState state);

    /**
     * Allocate pages for required size. If success, the page index will be return and the real size
     * will be assigned into *allocSize.
     * @param size, the required size, page of count.
     * @param allocSize, the real size allocated.
     * @return, page index for this allocation.
     */
    virtual int AllocPages(int size, int *allocSize);

    /**
     * Get the allocated size for this page.
     * @param page, page to find.
     * @return, the size to be allocated with this page.
     */
    virtual int GetAllocPageSize(int page);

    /**
     * Deallocate pages.
     * @param page, start page to deallocate.
     */
    virtual void DeallocPages(int page);

    /**
     * Allocate a linear storage space with specified length.
     * @param length, the length required to allocate.
     * @return a tmq address describe the allocated linear space.
     */
    virtual TMQAddress Allocate(TMQLSize length);

    /**
     * Deallocate a tmq address that is allocated by Allocate().
     * @param address, a tmq address to deallocate.
     */
    virtual void Deallocate(TMQAddress address);

    /**
     * Read data from address and save the data to the result buffer. If success, the read length
     * will be return, otherwise, -1 will be return.
     * @param address, address to read.
     * @param buf, a pointer to the buffer to save the results.
     * @param length, length to read.
     * @return TMQLSize, a TMQLSize value that has read.
     */
    virtual TMQLSize Read(TMQAddress address, void *buf, TMQLSize length);

    /**
     * Write data into a tmq address.
     * @param address, the address to write to.
     * @param data, a pointer to the data.
     * @param length, the length of the data.
     * @return, TMQLSize, the written length.
     */
    virtual TMQLSize Write(TMQAddress address, void *data, TMQLSize length);

    /**
     * Copy data for source address to the destination address in the linear space.
     * @param dst, destination address to copy to.
     * @param src, source address to copy from.
     * @param length, the length to copy.
     */
    virtual void Copy(TMQAddress dst, TMQAddress src, TMQLSize length);

    /**
     * Zero a tmq address with specified length.
     * @param address, a tmq address
     * @param length, the length to set zero.
     */
    virtual void Zero(TMQAddress address, TMQLSize length);
};


#endif //SECTION_SPACE_H
