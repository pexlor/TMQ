//
//  MemSpace.cpp
//  MemSpace
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "MemSpace.h"
#include "Defines.h"
#include "Metas.h"
#include <cstdlib>
#include <cstring>
#include <cstdlib>


/*
 * Construct a memory space, Initialize members.
 */
MemSpace::MemSpace() : pages{nullptr}, count(0), pageLens{0} {

}

/*
 * Allocate pages. If start is invalid ( < 0), it will find a free page index for this allocation.
 */
int MemSpace::Allocate(int start, int len) {
    // Allocation reaches to the MEMORY_PAGE_COUNT, failed.
    if (count >= MEMORY_PAGE_COUNT) {
        return -1;
    }
    // If the start is invalid or not specified, find a free page index for it.
    int index = start;
    if (index < 0) {
        for (int i = 0; i < MEMORY_PAGE_COUNT; ++i) {
            if (pages[i] == nullptr) {
                index = i;
                break;
            }
        }
    }
    // Check whether the page at index has allocated, if it has allocated, Deallocate it firstly.
    if (pages[index]) {
        Deallocate(index);
    }
    // Apply memory with calloc directly.
    pages[index] = static_cast<char *>(calloc(len * TMQ_PAGE_SIZE, sizeof(char)));
    // Check whether the calloc is success or not. If success return valid page index, otherwise
    // return invalid page index.
    if (pages[index]) {
        pageLens[index] = len;
        count++;
    } else {
        index = -1;
    }
    return index;
}

/*
 * Deallocate pages at page index.
 */
void MemSpace::Deallocate(int page) {
    // Check whether the page at index is valid.
    if (page >= 0 && pages[page]) {
        free(pages[page]);
        pages[page] = nullptr;
        count--;
    }
}

/*
 * Read data from page + offset to the buf.
 */
int MemSpace::Read(int page, int offset, void *buf, int len) {
    // Check parameters.
    if (page < 0 || page >= count || offset < 0 || !buf || len < 0) {
        return -1;
    }
    if (offset + len > pageLens[page] * TMQ_PAGE_SIZE) {
        abort();
    }
    // Copy data with memcpy.
    memcpy(buf, pages[page] + offset, len);
    return len;
}

/*
 * Write data to page + offset.
 */
int MemSpace::Write(int page, int offset, void *buf, int len) {
    // Check parameters.
    if (page >= 0 && offset >= 0 && buf && len > 0) {
        if (offset + len > pageLens[page] * TMQ_PAGE_SIZE) {
            abort();
        }
        // Copy data with memcpy.
        memcpy(pages[page] + offset, buf, len);
    }
    return len;
}

/*
 * Copy data from source page + offset ot destination page + offset.
 */
bool MemSpace::Copy(int dp, int df, int sp, int sf, int len) {
    bool success = false;
    if (df + len > pageLens[dp] * TMQ_PAGE_SIZE) {
        abort();
    }
    if (sf + len > pageLens[sp] * TMQ_PAGE_SIZE) {
        abort();
    }
    // check parameters and copy.
    if (dp >= 0 && df >= 0 && sp >= 0 && sf >= 0 && len >= 0) {
        memcpy(pages[dp] + df, pages[sp] + sf, len);
        success = true;
    }
    return success;
}

/*
 * Initialize the storage at page + offset
 */
void MemSpace::Zero(int page, int offset, int len) {
    // Check parameters.
    if (page >= 0 && offset >= 0 && len > 0) {
        memset(pages[page] + offset, 0, len);
    }
}

/*
 * Destruct the memory space. Release all memory in this function.
 */
MemSpace::~MemSpace() {
    for (int i = 0; i < MEMORY_PAGE_COUNT; ++i) {
        if (pages[i]) {
            free(pages[i]);
            pages[i] = nullptr;
        }
    }
    count = 0;
}



