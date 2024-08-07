
#ifndef MEMSPACE_H
#define MEMSPACE_H

#include "PageSpace.h"

/// 常量定义
// 定义 MemSpace 可以申请的最多页面数。
#define MEMORY_PAGE_COUNT 1280

/**
 * 使用内存存储实现 IPageSpace。在使用此实现之前，请注意，在关闭或设备断电时，内存存储中的数据将丢失。
 * 因此，在进行持久化实现时，MemSpace 只能在没有磁盘（文件）空间的情况下临时存储数据。
 */
class MemSpace : public IPageSpace {
private:
    // 内存中的活动页面数。
    int count;
    // 分配的页面。
    char *pages[MEMORY_PAGE_COUNT];
    // 记录 pages 中每个页面的页面数。
    int pageLens[MEMORY_PAGE_COUNT];
public:
    /**
     * 默认构造函数。
     */
    MemSpace();

    /**
     * 默认析构函数。
     */
    ~MemSpace();

    /**
     * 重写分配页面的方法。
     * @param start, 所需的页面索引。
     * @param len, 页面数。
     * @return, 分配的实际页面索引。
     */
    virtual int Allocate(int start, int len);

    /**
     * 释放页面。
     * @param page, 要释放的页面
     */
    virtual void Deallocate(int page);

    /*
     * 从内存空间读取数据。
     */
    virtual int Read(int page, int offset, void *buf, int len);

    /*
     * 将数据写入内存空间。
     */
    virtual int Write(int page, int offset, void *buf, int len);

    /*
     * 在内存空间中复制数据。
     */
    virtual bool Copy(int dp, int df, int sp, int sf, int len);

    /*
     * 用零初始化内存空间。
     */
    virtual void Zero(int page, int offset, int len);
};

#endif //MEMSPACE_H