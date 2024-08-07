#ifndef SECTION_ALLOCATOR_H
#define SECTION_ALLOCATOR_H

#include "Metas.h"
#include "RbTree.h"
#include "Persistence.h"

/// 常量定义
// 部分地址定义。部分地址由元素的索引和其 allocId 组成。
// 部分地址也是 TMQAddress 类型，长度为 64 位。高 32 位表示分配索引，而低 32 位表示 allocId。
#define SECTION_INDEX(address) (unsigned int)(address >> 32)
#define SECTION_ALLOC_ID(address) (unsigned int)(address)
#define SECTION_ADDRESS(index, allocId) ((TMQAddress)index << 32 | (TMQAddress)allocId)

/**
 * 部分分配器是 ISectionSpace 的部分实现，用于分配和
 * 释放线性空间。部分分配器最重要的能力是进行地址重用。
 * 部分分配器的第二个重要益处是它用添加操作替换了懒惰线性列表中元素的删除操作，
 * 这可以大大提高效率。
 */
class SectionAllocator : public ISectionSpace {
private:
    // 一个 RbTree，用于按 tmq 地址的升序存储释放的分配。
    RbTree<TMQAddress, SecAlloc> freedAllocTree;

protected:
    /**
     * 内部方法，用于地址重用。
     * @param size, 要重新分配的大小。
     * @return, 重用的 tmq 地址。
     */
    TMQAddress ReuseAddress(TMQSize size);

    /**
     * 通过部分地址查找 MetaAlloc。
     * @param secAddress, 方法 Allocate 的部分地址。
     * @param metaAlloc, 用于保存结果的 MetaAlloc。
     * @return 在线性空间中找到的元素的索引。
     */
    int FindAlloc(TMQAddress secAddress, SecAlloc &metaAlloc);

    /**
     * 内部方法，读取释放的分配并将它们放入 freedAllocTree。
     * 有时，从磁盘文件实现的线性空间读取数据将是耗时的。
     * 因此，此方法将批量读取数据。
     */
    void GetFreedAllocList();

    /**
     * 尝试释放页面。它将根据 tmq 地址计算释放的页面。
     * 当释放一个 tmq 地址时，我们将收集与同一页面相关的释放的 tmq 地址。
     * 如果所有地址片段都已释放，其关联的页面也将被释放。
     * @param address, 要释放的 tmq 地址。
     */
    bool TryFreePage(TMQAddress address);

    /**
     * 获取 SecAlloc 元素类型的懒惰线性列表的方法。
     * @param reserve, 此要求的保留计数。
     * @return, 懒惰线性列表的指针。
     */
    virtual LazyLinearList<SecAlloc> *GetLazyAllocList(TMQSize reserve = 0) = 0;

    /**
     * 分配页面的虚拟方法。
     * @param size, 所需的页面大小。
     * @param allocSize, 要分配的实际大小。
     * @return 页面索引。
     */
    virtual int AllocPages(int size, int *allocSize) = 0;

    /**
     * 获取已分配页面的大小。
     * @param page, 要查找的页面索引。
     * @return, 已分配页面的大小。
     */
    virtual int GetAllocPageSize(int page) = 0;

    /**
     * 释放页面。
     * @param page, 要释放的页面。
     */
    virtual void DeallocPages(int page) = 0;

    /**
     * 释放已分配的部分地址。
     * @param secAddress, 要释放的部分地址。
     * @return 表示释放是否成功的布尔值。
     */
    virtual bool ReleaseAlloc(TMQAddress secAddress) = 0;

    /**
     * 将分配信息追加到部分空间。
     * @param tmqAddress, 来自线性空间的 tmq 地址。
     * @param size, 地址的大小。
     * @param state, 此地址的分配状态：ADDRESS_ALLOC, ADDRESS_FREE, ADDRESS_RELEASE
     * @return 部分中的 tmq 地址。
     */
    virtual TMQAddress AppendAlloc(TMQAddress tmqAddress, TMQSize size, TMQLState state) = 0;

public:
    /**
     * 分配指定长度的线性存储空间。
     * @param length, 要分配的线性存储空间的长度。
     * @return, tmq 地址，如果分配失败，将返回 ADDRESS_NULL。
     */
    virtual TMQAddress Allocate(TMQLSize length);

    /**
     * 释放 tmq 地址。
     * @param address, 要释放的地址。
     */
    virtual void Deallocate(TMQAddress address);
};


#endif //SECTION_ALLOCATOR_H