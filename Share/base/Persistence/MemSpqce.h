//
//  MemSpace.h
//  MemSpace
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef MEMSPACE_H
#define MEMSPACE_H

#include "PageSpace.h"

/// Const definitions
// Define the max page count the MemSpace can apply.
#define MEMORY_PAGE_COUNT 1280

/**
 * Implement IPageSpace with memory storage. Before using this implementation, you should note that
 * the data in memory storage will be lost on shutting down or device power off. So upon the
 * persistence implementation, MemSpace can only use to store data temporarily when there are no any
 * disk(file) space.
 */
class MemSpace : public IPageSpace {
private:
    // Active page count in memory.
    int count;
    // Pages that are allocated.
    char *pages[MEMORY_PAGE_COUNT];
    // Page count of every page recorded in pages.
    int pageLens[MEMORY_PAGE_COUNT];
public:
    /**
     * Default constructor.
     */
    MemSpace();

    /**
     * Default destructor.
     */
    ~MemSpace();

    /**
     * Override method for allocating pages.
     * @param start, page index required.
     * @param len, count of pages.
     * @return, the real page index allocated.
     */
    virtual int Allocate(int start, int len);

    /**
     * Deallocate pages.
     * @param page, page to deallocate
     */
    virtual void Deallocate(int page);

    /*
     * Read data from memory space.
     */
    virtual int Read(int page, int offset, void *buf, int len);

    /*
     * Write data to memory space.
     */
    virtual int Write(int page, int offset, void *buf, int len);

    /*
     * Copy data in memory space.
     */
    virtual bool Copy(int dp, int df, int sp, int sf, int len);

    /*
     * Initialize memory space with zero.
     */
    virtual void Zero(int page, int offset, int len);
};


#endif //MEMSPACE_H
