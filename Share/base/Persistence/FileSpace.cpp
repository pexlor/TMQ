//
//  FileSpace.cpp
//  FileSpace
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "Defines.h"
#include "FileSpace.h"
#include "cstring"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>

/*
 * Apply new file space. If the require space exceeds the length of the fill, the file will be
 * expanded to fulfill the request. The expanded content will be fill with zero. If the expansion is
 * fail, the length of the file will be recovered.
 */
int FileSpace::Allocate(int start, int count) {
    // Check parameter
    if (count <= 0) {
        return PAGE_NULL;
    }
    // Calculate the start page index for the allocation.
    int realStart = (int)((start < 0) ? (length / TMQ_PAGE_SIZE) : start);
    // Calculate the final length of the file.
    TMQLSize require = (realStart + count) * TMQ_PAGE_SIZE;
    // Expand the file and fill the new content.
    if (require > length && truncate(file, (long)require) == 0) {
        // If the Fill action is not success, recover it.
        if (!Fill((long)length, (long)(require - length))) {
            truncate(file, (long)length);
            return PAGE_NULL;
        }
        // Expand success.
        length = require;
    }
    return realStart;
}

/*
 * Deallocate page content, truncate file for shrinking.
 */
void FileSpace::Deallocate(int page) {
    if (page >= 0) {
        long newLen = page * TMQ_PAGE_SIZE;
        // Set the new file length.
        if (truncate(file, newLen) == 0) {
            length = newLen;
        }
    }
}

/*
 *  Use Load method to achieve the memory address for the access content firstly, and then copy data
 *  using memcpy.
 */
int FileSpace::Read(int page, int offset, void *buf, int len) {
    // Check parameters.
    if (page < 0 || offset < 0 || !buf || len <= 0) {
        return -1;
    }
    // Read has reached or exceeded the end of the file, fail.
    if (page * TMQ_PAGE_SIZE + offset + len > length) {
        return -1;
    }
    int rp = page + offset / TMQ_PAGE_SIZE;
    int ro = offset % TMQ_PAGE_SIZE;
    int size = len;
    char *ptr = (char *) buf;
    // Read data from pages, until read count reaches required size.
    while (size > 0) {
        int count = TMQ_PAGE_SIZE - ro;
        if (count > size) {
            count = size;
        }
        char *data = (char *) Load(rp++);
        // copy data to result.
        memcpy(ptr, data + ro, count);
        size = size - count;
        ro = 0;
        ptr += count;
    }
    return len;
}

/*
 *  Use Load method to achieve the memory address for the access content firstly, and then copy data
 *  using memcpy.
 */
int FileSpace::Write(int page, int offset, void *buf, int len) {
    // Check parameters.
    if (page < 0 || offset < 0 || !buf || len < 0) {
        return -1;
    }
    int rp = page + offset / TMQ_PAGE_SIZE;
    int ro = offset % TMQ_PAGE_SIZE;
    int size = len;
    char *ptr = (char *) buf;
    // Write data to pages, until written count reaches the required size.
    while (size > 0) {
        int count = TMQ_PAGE_SIZE - ro;
        if (count > size) {
            count = size;
        }
        char *data = (char *) Load(rp++);
        memcpy(data + ro, ptr, count);
        size = size - count;
        ro = 0;
        ptr = ptr + count;
    }
    return len;
}

/*
 * Copy data from source to destination. This function will use mmap directly, and no cache reserved.
 */
bool FileSpace::Copy(int dp, int df, int sp, int sf, int len) {
    int fd = open(file, O_RDWR);
    if (fd <= 0) {
        return false;
    }
    bool success = false;
    int rdp = dp + df / TMQ_PAGE_SIZE;
    int rdf = df % TMQ_PAGE_SIZE;
    int rsp = sp + sf / TMQ_PAGE_SIZE;
    int rsf = sf % TMQ_PAGE_SIZE;
    // map the destination content.
    void *dst = mmap(nullptr, len, PROT_WRITE | PROT_READ, MAP_SHARED, fd, rdp * TMQ_PAGE_SIZE);
    // map the source content.
    void *src = mmap(nullptr, len, PROT_WRITE | PROT_READ, MAP_SHARED, fd, rsp * TMQ_PAGE_SIZE);
    // mmap finish, colse the file descriptor.
    close(fd);
    // Check and copy content when mmap is success on both source and destination content.
    if (dst != MAP_FAILED && src != MAP_FAILED) {
        memcpy((char *) dst + rdf, (char *) src + rsf, len);
        success = true;
    }
    // Concel mmap.
    if (dst != MAP_FAILED) {
        munmap(dst, len);
    }
    if (src != MAP_FAILED) {
        munmap(src, len);
    }
    return success;
}

/*
 * Set the content to zero. Use Load method to achieve the memory address for the access content
 * firstly, and then set zero with memset.
 */
void FileSpace::Zero(int page, int offset, int len) {
    if (page < 0 || offset < 0 || len < 0) {
        return;
    }
    int rp = page + offset / TMQ_PAGE_SIZE;
    int ro = offset % TMQ_PAGE_SIZE;
    int size = len;
    // Set values to pages, until the count reaches the required size.
    while (size > 0) {
        int count = TMQ_PAGE_SIZE - ro;
        if (count > size) {
            count = size;
        }
        char *data = (char *) Load(rp++);
        // Set to zero.
        memset(data + ro, 0, count);
        size = size - count;
        ro = 0;
    }
}

/*
 * Construct a file space. It will try to access the file, if it is not exist, it will be created.
 */
FileSpace::FileSpace(const char *path) :
        file{0}, pages{0}, maps{nullptr}, length(0) {
    // path is valid.
    if (path) {
        strncpy(file, path, sizeof(file));
        // Access the file, and use correct mode to open it.
        int fd = -1;
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        if (access(file, F_OK) != 0) {
            // Open with create mode.
            fd = open(file, O_RDWR | O_CREAT, mode);
        }
        if (fd <= 0) {
            // Open with read only mode.
            fd = open(file, O_RDONLY, mode);
        }
        if (fd > 0) {
            // Get the file length.
            length = lseek(fd, 0, SEEK_END);
            close(fd);
        }
    }
}

/*
 * Load data at page. If it is already loaded, use the address directly, otherwise, map the content
 * to memory with mmap.
 */
void *FileSpace::Load(int page) {
    if ((page + 1) * TMQ_PAGE_SIZE > length) {
        return nullptr;
    }
    // Calculate the position in pages with % operation.
    int pos = page % RESERVE_COUNT;
    // Check whether the pages[pos] is equal to the page required, If it is not equal, drop this
    // page.
    if (pages[pos] != page) {
        if (maps[pos]) {
            munmap(maps[pos], TMQ_PAGE_SIZE);
            maps[pos] = nullptr;
        }
    }
    // The page required is not loaded yet, open file and load it with mmap.
    if (!maps[pos]) {
        int fd = open(file, O_RDWR);
        // Open file with O_RDWR success.
        if (fd > 0) {
            maps[pos] = mmap(nullptr, TMQ_PAGE_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd,
                             page * TMQ_PAGE_SIZE);
            close(fd);
        }
        // Open file fail, means the load is fail.
        if (maps[pos] == MAP_FAILED) {
            maps[pos] = nullptr;
        }
    }
    pages[pos] = page;
    return maps[pos];
}

/*
 * Fill content with zero. This will use the safe IO operation to fill data to the content.
 */
bool FileSpace::Fill(long pos, long len) {
    // Check valid parameters.
    if (pos < 0 || len < 0) {
        return false;
    }
    bool suc = false;
    // Open file with read/write permissions.
    FILE *f = fopen(file, PAGE_FILE_MODE);
    if (f) {
        // Open success, use fwrite to write zeros to the required content.
        size_t size = len;
        fpos_t offset = pos;
        char zero[TMQ_PAGE_SIZE] = {0};
        fsetpos(f, &offset);
        while (size > 0) {
            size_t ws = size > sizeof(zero) ? sizeof(zero) : size;
            ws = fwrite(zero, 1, ws, f);
            // Write reaches to the required length.
            if (ws < 0) {
                break;
            }
            size -= ws;
        }
        // Close the file.
        fclose(f);
        // Write reaches to the required length, the result is success.
        suc = (size == 0);
    }
    return suc;
}
