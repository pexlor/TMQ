#include "Defines.h"
#include "FileSpace.h"
#include "cstring"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>

/*
 * 应用新的文件空间。如果所需空间超过文件的当前长度，文件将被扩展以满足请求。
 * 扩展的内容将用零填充。如果扩展失败，文件长度将恢复。
 */
int FileSpace::Allocate(int start, int count) {
    // 检查参数
    if (count <= 0) {
        return PAGE_NULL;
    }
    // 计算分配的起始页面索引。
    int realStart = (int)((start < 0) ? (length / TMQ_PAGE_SIZE) : start);
    // 计算文件的最终长度。
    TMQLSize require = (realStart + count) * TMQ_PAGE_SIZE;
    // 扩展文件并填充新内容。
    if (require > length && truncate(file, (long)require) == 0) {
        // 如果填充操作不成功，恢复它。
        if (!Fill((long)length, (long)(require - length))) {
            truncate(file, (long)length);
            return PAGE_NULL;
        }
        // 扩展成功。
        length = require;
    }
    return realStart;
}

/*
 * 释放页面内容，截断文件以缩小。
 */
void FileSpace::Deallocate(int page) {
    if (page >= 0) {
        long newLen = page * TMQ_PAGE_SIZE;
        // 设置新的文件长度。
        if (truncate(file, newLen) == 0) {
            length = newLen;
        }
    }
}

/*
 * 首先使用Load方法获取访问内容的内存地址，然后使用memcpy复制数据。
 */
int FileSpace::Read(int page, int offset, void *buf, int len) {
    // 检查参数。
    if (page < 0 || offset < 0 || !buf || len <= 0) {
        return -1;
    }
    // 读取已到达或超过文件末尾，失败。
    if (page * TMQ_PAGE_SIZE + offset + len > length) {
        return -1;
    }
    int rp = page + offset / TMQ_PAGE_SIZE;
    int ro = offset % TMQ_PAGE_SIZE;
    int size = len;
    char *ptr = (char *) buf;
    // 从页面读取数据，直到读取计数达到所需大小。
    while (size > 0) {
        int count = TMQ_PAGE_SIZE - ro;
        if (count > size) {
            count = size;
        }
        char *data = (char *) Load(rp++);
        // 将数据复制到结果中。
        memcpy(ptr, data + ro, count);
        size = size - count;
        ro = 0;
        ptr += count;
    }
    return len;
}

/*
 * 首先使用Load方法获取访问内容的内存地址，然后使用memcpy复制数据。
 */
int FileSpace::Write(int page, int offset, void *buf, int len) {
    // 检查参数。
    if (page < 0 || offset < 0 || !buf || len < 0) {
        return -1;
    }
    int rp = page + offset / TMQ_PAGE_SIZE;
    int ro = offset % TMQ_PAGE_SIZE;
    int size = len;
    char *ptr = (char *) buf;
    // 将数据写入页面，直到写入计数达到所需大小。
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
 * 将数据从源复制到目标。此函数将直接使用mmap，不保留缓存。
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
    // 映射目标内容。
    void *dst = mmap(nullptr, len, PROT_WRITE | PROT_READ, MAP_SHARED, fd, rdp * TMQ_PAGE_SIZE);
    // 映射源内容。
    void *src = mmap(nullptr, len, PROT_WRITE | PROT_READ, MAP_SHARED, fd, rsp * TMQ_PAGE_SIZE);
    // mmap完成，关闭文件描述符。
    close(fd);
    // 当源和目标内容的mmap都成功时，检查和复制内容。
    if (dst != MAP_FAILED && src != MAP_FAILED) {
        memcpy((char *) dst + rdf, (char *) src + rsf, len);
        success = true;
    }
    // 取消mmap。
    if (dst != MAP_FAILED) {
        munmap(dst, len);
    }
    if (src != MAP_FAILED) {
        munmap(src, len);
    }
    return success;
}

/*
 * 将内容设置为零。首先使用Load方法获取访问内容的内存地址，然后使用memset设置为零。
 */
void FileSpace::Zero(int page, int offset, int len) {
    if (page < 0 || offset < 0 || len < 0) {
        return;
    }
    int rp = page + offset / TMQ_PAGE_SIZE;
    int ro = offset % TMQ_PAGE_SIZE;
    int size = len;
    // 设置页面的值，直到计数达到所需大小。
    while (size > 0) {
        int count = TMQ_PAGE_SIZE - ro;
        if (count > size) {
            count = size;
        }
        char *data = (char *) Load(rp++);
        // 设置为零。
        memset(data + ro, 0, count);
        size = size - count;
        ro = 0;
    }
}

/*
 * 构造一个文件空间。它将尝试访问文件，如果不存在，将创建它。
 */
FileSpace::FileSpace(const char *path) :
    file{0}, pages{0}, maps{nullptr}, length(0) {
    // 路径有效。
    if (path) {
        strncpy(file, path, sizeof(file));
        // 访问文件，并以正确的模式打开它。
        int fd = -1;
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        if (access(file, F_OK) != 0) {
            // 以创建模式打开。
            fd = open(file, O_RDWR | O_CREAT, mode);
        }
        if (fd <= 0) {
            // 以只读模式打开。
            fd = open(file, O_RDONLY, mode);
        }
        if (fd > 0) {
            // 获取文件长度。
            length = lseek(fd, 0, SEEK_END);
            close(fd);
        }
    }
}

/*
 * 加载页面上的数据。如果已经加载，直接使用地址，否则，使用mmap将内容映射到内存。
 */
void *FileSpace::Load(int page) {
    if ((page + 1) * TMQ_PAGE_SIZE > length) {
        return nullptr;
    }
    // 使用%操作计算页面中的位置。
    int pos = page % RESERVE_COUNT;
    // 检查pages[pos]是否等于所需的页面，如果不等于，丢弃这个页面。
    if (pages[pos] != page) {
        if (maps[pos]) {
            munmap(maps[pos], TMQ_PAGE_SIZE);
            maps[pos] = nullptr;
        }
    }
    // 所需的页面尚未加载，打开文件并用mmap加载它。
    if (!maps[pos]) {
        int fd = open(file, O_RDWR);
        // 成功以O_RDWR打开文件。
        if (fd > 0) {
            maps[pos] = mmap(nullptr, TMQ_PAGE_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd,
                            page * TMQ_PAGE_SIZE);
            close(fd);
        }
        // 打开文件失败，意味着加载失败。
        if (maps[pos] == MAP_FAILED) {
            maps[pos] = nullptr;
        }
    }
    pages[pos] = page;
    return maps[pos];
}

/*
 * 用零填充内容。这将使用安全的IO操作将数据填充到内容中。
 */
bool FileSpace::Fill(long pos, long len) {
    // 检查有效参数。
    if (pos < 0 || len < 0) {
        return false;
    }
    bool suc = false;
    // 以读/写权限打开文件。
    FILE *f = fopen(file, PAGE_FILE_MODE);
    if (f) {
        // 打开成功，使用fwrite将零写入所需内容。
        size_t size = len;
        fpos_t offset = pos;
        char zero[TMQ_PAGE_SIZE] = {0};
        fsetpos(f, &offset);
        while (size > 0) {
            size_t ws = size > sizeof(zero) ? sizeof(zero) : size;
            ws = fwrite(zero, 1, ws, f);
            // 写入达到所需长度。
            if (ws < 0) {
                break;
            }
            size -= ws;
        }
        // 关闭文件。
        fclose(f);
        // 写入达到所需长度，结果是成功的。
        suc = (size == 0);
    }
    return suc;
}