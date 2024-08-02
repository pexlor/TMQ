//
//  FileSpace.h
//  FileSpace
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef FILE_SPACE_H
#define FILE_SPACE_H

#include "List.h"
#include "Metas.h"
#include "PageSpace.h"

/// Const definitions for file space.
// Max file path length
#define PATH_LENGTH             256
// Max page count caching for loaded pages.
#define RESERVE_COUNT           512
// File mode for opening a file. It will create a new file when the file is not exists.
#define PAGE_FILE_MODE          "rb+"

/**
 * FileSpace is an implementation for IPageSpace using a file on disk. The file hold by FileSpace
 * must has the read/write/create permissions. When the file is ready, we will use mmap to access
 * the raw data. Considering the efficiency, we will cache some pages in memory, and the max cached
 * count limits to RESERVE_COUNT.
 */
class FileSpace : public IPageSpace {
private:
    // Char array for saving file path.
    char file[PATH_LENGTH];
    // The real length of this file in bytes.
    TMQLSize length;
    // Index of pages that has already allocated.
    int pages[RESERVE_COUNT];
    // Cached memory address from mmap, the max count limits to RESERVE_COUNT.
    void *maps[RESERVE_COUNT];
public:
    /**
     * Constructor a FileSpace with a file path.
     * @param path, the path to the file. It must have the read, write and create permissions.
     */
    explicit FileSpace(const char *path);

    /**
     * Load a page and return its memory address. If the page has been loaded, it will return
     * immediately, otherwise, it will load the data from disk file using mmap or other IO method.
     * @param page, page index to load.
     * @return, a pointer to the memory address of this page.
     */
    void *Load(int page);

    /**
     * On linux, the content mapped by mmap is not owned by the mmap really, until it is read or
     * written. So it is very dangerous to read/write on a position that is not exist on the disk,
     * witch will cause SIG_BUS error. So, before mmap some contents, we should fill it with safe IO
     * operation.
     * @param pos, the start position to be filled.
     * @param len, the length to fill.
     * @return, a boolean value indicate whether the fill is success or not. If fail, we should not
     *  do mmap on it.
     */
    bool Fill(long pos, long len);

    /**
     * Allocate some continuous pages. If success, the page index will be return.
     * @param start, the start page index of requirement, if invalid( < 0), it will allocate pages
     *  randomly.
     * @param size, count of the continuous pages.
     * @return a int value of the real page index, which is above zero always for success allocating.
     */
    virtual int Allocate(int start, int size);

    /**
     * Deallocate pages at page index.
     * @param page, a page index, returned by Allocate
     */
    virtual void Deallocate(int page);

    /**
     * Read method for copying content to the memory buf.
     * @param page, page index to read
     * @param offset, offset on this page.
     * @param buf, a memory pointer to receive the content.
     * @param len, size to read.
     * @return the real length to be read.
     */
    virtual int Read(int page, int offset, void *buf, int len);

    /**
     * Write method for copy data from memory to linear storage space.
     * @param page , page index to write
     * @param offset, offset on this page.
     * @param buf, a memory pointer to copy data.
     * @param len, size to write.
     * @return the real length to be written.
     */
    virtual int Write(int page, int offset, void *buf, int len);

    /**
     * Copy data in the linear storage space. This method requires to copy data without memory.
     * @param dp, destination page index.
     * @param df, destination offset of the dp.
     * @param sp, source page index.
     * @param sf, source offset of the sp.
     * @param len, copy length, requires > 0
     * @return a boolean value indicates whether the copy is success or not.
     */
    virtual bool Copy(int dp, int df, int sp, int sf, int len);

    /**
     * Initialize the page spaces with zero.
     * @param page, page index to zero.
     * @param offset, offset on this page.
     * @param len, length to zero.
     */
    virtual void Zero(int page, int offset, int len);
};


#endif // FILE_SPACE_H
