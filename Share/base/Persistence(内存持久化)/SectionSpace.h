#ifndef SECTION_SPACE_H
#define SECTION_SPACE_H

#include "Metas.h"
#include "LinearSpace.h"
#include "Persistence.h"
#include "LazyLinkList.h"
#include "Ordered.h"
#include "SectionAllocator.h"

/// 常量定义
// 内部部分名称用于分配的页面。
#define SECTION_ALLOC "__ALLOC__"
// int 的最大分配 id。
#define ALLOC_ID_MAX 0xffffffff
#define SPACE_RESET_THRESHOLD 0.3
// 延迟线性列表的元素预留计数。
#define LAZY_RESERVE_READ 0
#define LAZY_RESERVE_WRITE 1

/**
 * SectionSpace 从 SectionAllocator 继承。使用此类，我们实现了 ISectionSpace 中定义的所有 API，
 * 如 Read、Write、Copy、Zero 和从其父级继承的其他虚拟函数。此外，SectionSpace 保存名称和所有 SecAlloc。
 * 我们使用 LazyLinearList 管理所有 SecAlloc，因此我们必须小心处理空间扩展和收缩。
 * 为了确保部分空间分配效率，不立即清理部分空间。常量 SPACE_RESET_THRESHOLD 用于切换 ResetSpace 任务。
 */
class SectionSpace : public SectionAllocator {
private:
    // 保存部分名称的数组。
    char name[SECTION_NAME_LEN];
    // 指向持久性的指针
    Persistence *persist;
    // 部分分配的 alloc id。
    unsigned int allocId = 0;
    // 释放的计数。
    unsigned int releaseCount;
    // 描述部分空间位置的元部分。
    MetaSection metaSection;
    // 部分分配列表信息。
    LazyLinearList<SecAlloc> lazyAllocList;

public:
    /**
     * 使用持久性和指向名称的指针构造 Section Space。
     * @param persist, 指向持久性的指针。
     * @param sec, 指向部分名称的指针。
     */
    SectionSpace(Persistence *persist, const char *sec);

    /**
     * 默认析构。
     */
    virtual ~SectionSpace();

    /**
     * 重置并清理部分空间。当此任务发生时，延迟线性列表将缩小。
     * 具有释放状态的 SecAlloc 将被移除。
     * @return SecAlloc 列表的大小。
     */
    unsigned int ResetSpace();

    /**
     * 获取部分名称的方法。
     * @return 部分名称的常量指针。
     */
    const char *GetName();

    /**
     * 从部分空间收集 MetaAlloc 列表的方法。
     * @param allocList, 用于保存结果的 MetaAlloc 列表引用
     * @return, 是否成功的布尔值。
     */
    virtual bool GetAllocList(List<MetaAlloc> &allocList);

    /**
     * 获取具有所需预留计数的延迟线性列表的指针。例如，如果您只想在此列表上进行读取操作，预留计数可以为零。
     * 或者，如果您想向列表中添加一个或多个元素，预留计数必须是您要添加的计数。
     * 当预留计数无法满足时，部分空间将自行扩大。
     * @param reserve, LazyLinearList 的预留计数以确保
     * @return, LazyLinearList 的指针。
     */
    virtual LazyLinearList<SecAlloc> *GetLazyAllocList(TMQSize reserve);

    /**
     * 释放 SecAlloc，这只会更改与此 tmq 部分地址关联的 SecAlloc 的状态，然后切换 ResetSpace 任务。
     * @param secAddress, 到 SecAlloc 的 tmq 地址。
     * @return, 表示释放是否成功的布尔值。
     */
    virtual bool ReleaseAlloc(TMQAddress secAddress);

    /**
     * 追加 SecAlloc 并返回部分地址。
     * @param tmqAddress, 到 MetaAlloc 的 tmq 地址。
     * @param size, 此地址的大小。
     * @param state, 此 SectionAlloc 的状态，ADDRESS_ALLOC, ADDRESS_FREE。
     * @return, 部分地址中的 tmq 地址。
     */
    virtual TMQAddress AppendAlloc(TMQAddress tmqAddress, TMQSize size, TMQLState state);

    /**
     * 为所需大小分配页面。如果成功，将返回页面索引并将实际大小分配给 *allocSize。
     * @param size, 所需大小，页面计数。
     * @param allocSize, 实际分配的大小。
     * @return, 此分配的页面索引。
     */
    virtual int AllocPages(int size, int *allocSize);

    /**
     * 获取为此页面分配的大小。
     * @param page, 要查找的页面。
     * @return, 为此页面分配的大小。
     */
    virtual int GetAllocPageSize(int page);

    /**
     * 释放页面。
     * @param page, 要释放的起始页面。
     */
    virtual void DeallocPages(int page);

    /**
     * 为指定长度分配线性存储空间。
     * @param length, 所需分配的长度。
     * @return 描述分配的线性空间的 tmq 地址。
     */
    virtual TMQAddress Allocate(TMQLSize length);

    /**
     * 释放由 Allocate() 分配的 tmq 地址。
     * @param address, 要释放的 tmq 地址。
     */
    virtual void Deallocate(TMQAddress address);

    /**
     * 从地址读取数据并将数据保存到结果缓冲区。如果成功，将返回读取长度，否则返回 -1。
     * @param address, 读取的地址。
     * @param buf, 保存结果的缓冲区指针。
     * @param length, 读取的长度。
     * @return TMQLSize, 已读取的 TMQLSize 值。
     */
    virtual TMQLSize Read(TMQAddress address, void *buf, TMQLSize length);

    /**
     * 将数据写入 tmq 地址。
     * @param address, 要写入的地址。
     * @param data, 数据的指针。
     * @param length, 数据的长度。
     * @return, TMQLSize, 写入的长度。
     */
    virtual TMQLSize Write(TMQAddress address, void *data, TMQLSize length);

    /**
     * 将源地址的数据复制到线性空间中的目标地址。
     * @param dst, 复制到的目标地址。
     * @param src, 从中复制的源地址。
     * @param length, 要复制的长度。
     */
    virtual void Copy(TMQAddress dst, TMQAddress src, TMQLSize length);

    /**
     * 用指定长度将 tmq 地址归零。
     * @param address, tmq 地址
     * @param length, 设置为零的长度。
     */
    virtual void Zero(TMQAddress address, TMQLSize length);
};


#endif //SECTION_SPACE_H