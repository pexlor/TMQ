//
//  PageSpace.h
//  PageSpace
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef PAGE_SPACE_H
#define PAGE_SPACE_H

/**
 * Interface definition for page space. We abstract the linear storage space as a continuous space
 * in units of pages. The reason we do this abstraction is that most of the popular os such as linux,
 * windows, or linux-based os support the paging storage management as a base feature. In order to
 * take advantages of the paging system on os, we make a IPageSpace interface as the abstract layer
 * between os and linear storage space of tmq. Linear storage media can be file on disk or flash
 * memory, all of these storage space will be mapped into operating system first and then tmq can
 * access them with the memory handler(IPageSpace*) efficiently.
 */

class IPageSpace {
public:
    /**
     * Allocate some continuous pages. If success, the page index will be return.
     * @param start, the start page index of requirement, if invalid( < 0), it will allocate pages
     *  randomly.
     * @param size, count of the continuous pages.
     * @return a int value of the real page index, which is above zero always for success allocating.
     */
    virtual int Allocate(int start, int size) = 0;

    /**
     * Deallocate pages at page index.
     * @param page, a page index, returned by Allocate
     */
    virtual void Deallocate(int page) = 0;

    /**
     * Read method for copying content to the memory buf.
     * @param page, page index to read
     * @param offset, offset on this page.
     * @param buf, a memory pointer to receive the content.
     * @param len, size to read.
     * @return the real length to be read.
     */
    virtual int Read(int page, int offset, void *buf, int len) = 0;

    /**
     * Write method for copy data from memory to linear storage space.
     * @param page , page index to write
     * @param offset, offset on this page.
     * @param buf, a memory pointer to copy data.
     * @param len, size to write.
     * @return the real length to be written.
     */
    virtual int Write(int page, int offset, void *buf, int len) = 0;

    /**
     * Copy data in the linear storage space. This method requires to copy data without memory.
     * @param dp, destination page index.
     * @param df, destination offset of the dp.
     * @param sp, source page index.
     * @param sf, source offset of the sp.
     * @param len, copy length, requires > 0
     * @return a boolean value indicates whether the copy is success or not.
     */
    virtual bool Copy(int dp, int df, int sp, int sf, int len) = 0;

    /**
     * Initialize the page spaces with zero.
     * @param page, page index to zero.
     * @param offset, offset on this page.
     * @param len, length to zero.
     */
    virtual void Zero(int page, int offset, int len) = 0;

    /**
     * Virtual destructor.
     */
    virtual ~IPageSpace() {}
};


#endif //PAGE_SPACE_H
