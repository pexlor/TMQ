//
// MemSpace.cpp
// MemSpace
//
// 创建于 2022/5/28.
// 版权所有 (c) 腾讯。保留所有权利。
//

#include "MemSpace.h"
#include "Defines.h"
#include "Metas.h"
#include <cstdlib>
#include <cstring>
#include <cstdlib>

/*
 * 构造一个内存空间，初始化成员。
 */
MemSpace::MemSpace() : pages{nullptr}, count(0), pageLens{0} {

}

/*
 * 分配页面。如果 start 无效（< 0），它将为此分配找到一个空闲页面索引。
 */
int MemSpace::Allocate(int start, int len) {
    // 分配达到 MEMORY_PAGE_COUNT，失败。
    if (count >= MEMORY_PAGE_COUNT) {
        return -1;
    }
    // 如果 start 无效或未指定，为它找到一个空闲页面索引。
    int index = start;
    if (index < 0) {
        for (int i = 0; i < MEMORY_PAGE_COUNT; ++i) {
            if (pages[i] == nullptr) {
                index = i;
                break;
            }
        }
    }
    // 检查位于索引处的页面是否已分配，如果是，则首先释放它。
    if (pages[index]) {
        Deallocate(index);
    }
    // 直接使用 calloc 申请内存。
    pages[index] = static_cast<char *>(calloc(len * TMQ_PAGE_SIZE, sizeof(char)));
    // 检查 calloc 是否成功。如果成功返回有效页面索引，否则返回无效页面索引。
    if (pages[index]) {
        pageLens[index] = len;
        count++;
    } else {
        index = -1;
    }
    return index;
}

/*
 * 在页面索引处释放页面。
 */
void MemSpace::Deallocate(int page) {
    // 检查位于索引处的页面是否有效。
    if (page >= 0 && pages[page]) {
        free(pages[page]);
        pages[page] = nullptr;
        count--;
    }
}

/*
 * 从 page + offset 读取数据到 buf。
 */
int MemSpace::Read(int page, int offset, void *buf, int len) {
    // 检查参数。
    if (page < 0 || page >= count || offset < 0 || !buf || len < 0) {
        return -1;
    }
    if (offset + len > pageLens[page] * TMQ_PAGE_SIZE) {
        abort();
    }
    // 使用 memcpy 复制数据。
    memcpy(buf, pages[page] + offset, len);
    return len;
}

/*
 * 将数据写入 page + offset。
 */
int MemSpace::Write(int page, int offset, void *buf, int len) {
    // 检查参数。
    if (page >= 0 && offset >= 0 && buf && len > 0) {
        if (offset + len > pageLens[page] * TMQ_PAGE_SIZE) {
            abort();
        }
        // 使用 memcpy 复制数据。
        memcpy(pages[page] + offset, buf, len);
    }
    return len;
}

/*
 * 从源页面 + 偏移复制数据到目标页面 + 偏移。
 */
bool MemSpace::Copy(int dp, int df, int sp, int sf, int len) {
    bool success = false;
    if (df + len > pageLens[dp] * TMQ_PAGE_SIZE) {
        abort();
    }
    if (sf + len > pageLens[sp] * TMQ_PAGE_SIZE) {
        abort();
    }
    // 检查参数并复制。
    if (dp >= 0 && df >= 0 && sp >= 0 && sf >= 0 && len >= 0) {
        memcpy(pages[dp] + df, pages[sp] + sf, len);
        success = true;
    }
    return success;
}

/*
 * 初始化位于 page + offset 的存储。
 */
void MemSpace::Zero(int page, int offset, int len) {
    // 检查参数。
    if (page >= 0 && offset >= 0 && len > 0) {
        memset(pages[page] + offset, 0, len);
    }
}

/*
 * 析构内存空间。在此函数中释放所有内存。
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