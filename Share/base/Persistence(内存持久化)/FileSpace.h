#ifndef FILE_SPACE_H
#define FILE_SPACE_H

#include "List.h"
#include "Metas.h"
#include "PageSpace.h"

/// 文件空间的常量定义。
// 文件路径最大长度
#define PATH_LENGTH 256
// 缓存已加载页面的最大页面计数。
#define RESERVE_COUNT 512
// 打开文件的文件模式。当文件不存在时，将创建一个新文件。
#define PAGE_FILE_MODE "rb+"

/**
 * FileSpace 是使用磁盘上文件的 IPageSpace 实现。FileSpace 持有的文件必须具有读/写/创建权限。
 * 当文件准备就绪时，我们将使用 mmap 访问原始数据。考虑到效率，我们将在内存中缓存一些页面，最大缓存计数限制为 RESERVE_COUNT。
 */
class FileSpace : public IPageSpace {
private:
    // 用于保存文件路径的字符数组。
    char file[PATH_LENGTH];
    // 文件的实际字节长度。
    TMQLSize length;
    // 已分配页面的索引。
    int pages[RESERVE_COUNT];
    // 从 mmap 缓存的内存地址，最大计数限制为 RESERVE_COUNT。
    void *maps[RESERVE_COUNT];

public:
    /**
     * 使用文件路径构造 FileSpace。
     * @param path, 文件的路径。它必须具有读、写和创建权限。
     */
    explicit FileSpace(const char *path);

    /**
     * 加载页面并返回其内存地址。如果页面已加载，它将立即返回，否则，它将使用 mmap 或其他 IO 方法从磁盘文件加载数据。
     * @param page, 要加载的页面索引。
     * @return, 指向该页面内存地址的指针。
     */
    void *Load(int page);

    /**
     * 在 Linux 上，通过 mmap 映射的内容实际上并不属于 mmap，直到它被读取或写入。因此，在磁盘上不存在的位置进行读/写是非常危险的，
     * 这将导致 SIG_BUS 错误。因此，在 mmap 一些内容之前，我们应该使用安全的 IO 操作填充它。
     * @param pos, 要填充的起始位置。
     * @param len, 要填充的长度。
     * @return, 表示填充是否成功的布尔值。如果失败，我们不应该在其上进行 mmap。
     */
    bool Fill(long pos, long len);

    /**
     * 分配一些连续的页面。如果成功，将返回页面索引。
     * @param start, 所需的起始页面索引，如果无效 (< 0)，将随机分配页面。
     * @param size, 连续页面的数量。
     * @return 成功分配的实际页面索引的整数值，始终大于零。
     */
    virtual int Allocate(int start, int size);

    /**
     * 在页面索引处释放页面。
     * @param page, 通过 Allocate 返回的页面索引。
     */
    virtual void Deallocate(int page);

    /**
     * 读取方法，用于将内容复制到内存 buf。
     * @param page, 要读取的页面索引。
     * @param offset, 此页面上的偏移量。
     * @param buf, 接收内容的内存指针。
     * @param len, 要读取的大小。
     * @return 实际读取的长度。
     */
    virtual int Read(int page, int offset, void *buf, int len);

    /**
     * 写入方法，用于将数据从内存复制到线性存储空间。
     * @param page, 要写入的页面索引。
     * @param offset, 此页面上的偏移量。
     * @param buf, 要复制数据的内存指针。
     * @param len, 要写入的大小。
     * @return 实际写入的长度。
     */
    virtual int Write(int page, int offset, void *buf, int len);

    /**
     * 在线性存储空间中复制数据。此方法要求在不使用内存的情况下复制数据。
     * @param dp, 目标页面索引。
     * @param df, dp 的目标偏移量。
     * @param sp, 源页面索引。
     * @param sf, sp 的源偏移量。
     * @param len, 复制长度，要求 > 0
     * @return 表示复制是否成功的布尔值。
     */
    virtual bool Copy(int dp, int df, int sp, int sf, int len);

    /**
     * 使用零初始化页面空间。
     * @param page, 要置零的页面索引。
     * @param offset, 此页面上的偏移量。
     * @param len, 要置零的长度。
     */
    virtual void Zero(int page, int offset, int len);
};

#endif // FILE_SPACE_H