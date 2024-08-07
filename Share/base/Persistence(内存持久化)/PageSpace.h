//
// PageSpace.h
// PageSpace
//
// 创建于 2022/5/28.
// 版权所有 (c) 腾讯。保留所有权利。
//

#ifndef PAGE_SPACE_H
#define PAGE_SPACE_H

/**
 * 页面空间的接口定义。我们将线性存储空间抽象为以页面为单位的连续空间。
 * 我们这样做抽象的原因是，大多数流行的操作系统（如 linux、windows 或基于 linux 的操作系统）
 * 都支持作为基本功能的分页存储管理。为了利用操作系统上的分页系统，
 * 我们创建了 IPageSpace 接口作为操作系统和 tmq 线性存储空间之间的抽象层。
 * 线性存储介质可以是磁盘上的文件或闪存，所有这些存储空间都将首先映射到操作系统中，
 * 然后 tmq 可以通过内存处理程序（IPageSpace*）高效地访问它们。
 */

class IPageSpace {
public:
    /**
     * 分配一些连续的页面。如果成功，将返回页面索引。
     * @param start, 所需的起始页面索引，如果无效（< 0），将随机分配页面。
     * @param size, 连续页面的计数。
     * @return 分配成功的实际页面索引的整数值，始终大于零。
     */
    virtual int Allocate(int start, int size) = 0;

    /**
     * 在页面索引处释放页面。
     * @param page, 通过 Allocate 返回的页面索引。
     */
    virtual void Deallocate(int page) = 0;

    /**
     * 读取方法，用于将内容复制到内存 buf。
     * @param page, 要读取的页面索引。
     * @param offset, 此页面上的偏移量。
     * @param buf, 接收内容的内存指针。
     * @param len, 要读取的大小。
     * @return 实际读取的长度。
     */
    virtual int Read(int page, int offset, void *buf, int len) = 0;

    /**
     * 写入方法，用于将数据从内存复制到线性存储空间。
     * @param page, 要写入的页面索引。
     * @param offset, 此页面上的偏移量。
     * @param buf, 要复制数据的内存指针。
     * @param len, 要写入的大小。
     * @return 实际写入的长度。
     */
    virtual int Write(int page, int offset, void *buf, int len) = 0;

    /**
     * 在线性存储空间中复制数据。此方法要求在不使用内存的情况下复制数据。
     * @param dp, 目标页面索引。
     * @param df, dp 的目标偏移量。
     * @param sp, 源页面索引。
     * @param sf, sp 的源偏移量。
     * @param len, 复制长度，要求 > 0
     * @return 表示复制是否成功的布尔值。
     */
    virtual bool Copy(int dp, int df, int sp, int sf, int len) = 0;

    /**
     * 使用零初始化页面空间。
     * @param page, 要置零的页面索引。
     * @param offset, 此页面上的偏移量。
     * @param len, 要置零的长度。
     */
    virtual void Zero(int page, int offset, int len) = 0;

    /**
     * 虚析构函数。
     */
    virtual ~IPageSpace() {}
};

#endif //PAGE_SPACE_H