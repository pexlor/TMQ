#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "Metas.h"
#include "LinearSpace.h"
#include "PageSpace.h"
#include "LazyLinearList.h"
#include "List.h"

/**
 * 数据持久性的接口定义。它提供了管理部分空间的函数，如创建、删除和查找方法。
 * 其他函数，EraseLinearSpace、AppendLinearSpace 也是管理部分空间的 API。
 * 有了部分空间，可以根据需要分配、释放线性空间，并从/向持久性读取/写入数据。
 */
class IPersistence {
public:
    /**
     * 创建一个命名的线性空间。
     * @param name, 名称的指针。
     * @return, 创建的部分空间的指针。
     */
    virtual ISectionSpace *CreateLinearSpace(const char *name) = 0;

    /**
     * 删除指定名称的线性空间。
     * @param name, 名称的指针。
     */
    virtual void DropLinearSpace(const char *name) = 0;

    /**
     * 查找指定名称的线性空间。
     * @param name, 名称的指针。
     * @return 如果不存在，则返回 nullptr 的部分空间指针。
     */
    virtual ISectionSpace *FindLinearSpace(const char *name) = 0;

    /**
     * 擦除线性空间，清除该部分空间中的所有数据。
     * @param name, 名称的指针。
     */
    virtual void EraseLinearSpace(const char *name) = 0;

    /**
     * 将源部分空间的数据复制到目标部分空间的末尾。
     * @param dst, 目标部分名称。
     * @param src, 源部分名称。
     * @return, bool, 表示成功与否的布尔值。
     */
    virtual bool AppendLinearSpace(const char *dst, const char *src) = 0;
};

/**
 * IPersistence 的实现。此类将用于管理页面空间和多个部分空间，
 * 并添加或删除元部分。
 */
class Persistence : public IPersistence {
private:
    // 页面空间的指针，MemSpace 或 FileSpace。
    IPageSpace *pageSpace;
    // 部分空间列表。
    List<ILinearSpace *> sectionSpaces;
    // 元部分列表。
    LazyLinearList<MetaSection> *lazySectionList;

public:
    /**
     * 使用页面空间构造持久性。
     * @param pageSpace, 页面空间的指针。
     */
    Persistence(IPageSpace *pageSpace);

    /**
     * Persistence 的析构函数。
     */
    ~Persistence();

    /**
     * 获取页面空间的方法。
     * @return 页面空间的指针。
     */
    IPageSpace *GetPageSpace();

    /**
     * 查找元部分，如果不存在，将创建元部分。
     * @param sec, 部分名称的指针。
     * @param create, 表示是否创建新元部分的布尔值。
     * @return 由 sec 命名的元部分。
     */
    MetaSection FindSection(const char *sec, bool create = true);

    /**
     * 擦除部分，清除名为 sec 的部分空间中的所有数据。
     * @param sec, 部分名称的常量指针。
     */
    void EraseSection(const char *sec);

    /**
     * 检查部分空间是否溢出。
     * @param section, 要检查的元部分。
     * @param expand, 要检查的保留计数。
     * @return, bool, 表示是否溢出保留计数的布尔值。
     */
    bool Overflow(const MetaSection &section, TMQSize expand);

    /**
     * 使用保留计数调整元部分的大小，默认保留为 1。
     * @param section, 要保留的元部分。
     * @param reserve, 要保留的计数。
     * @return bool, 表示调整大小操作是否成功的布尔值。
     */
    bool ResizeSection(MetaSection &section, TMQSize reserve = 1);

    /**
     * 更新元部分。这将释放持久性中元部分的页面，并更新到参数命名的部分中的页面。
     * @param section, 要更新的部分。
     * @return bool, 表示此更新是否成功的布尔值。
     */
    bool UpdateSection(MetaSection &section);

    /**
     * 移动元部分及其部分空间到新页面。
     * @param section, 要移动的元部分。
     * @param page, 新的目标页面。
     * @param size, 新页面的大小。
     * @return, bool, 表示移动是否成功的布尔值。
     */
    bool MoveSection(MetaSection &section, int page, TMQSize size);

    /**
     * 进行页面重用。当线性空间中间的页面被释放时，
     * 释放这些页面对线性空间的最终长度没有影响。
     * 因此，如果释放的页面在线性空间的中间，它不会立即释放，
     * 相反，它将被放入释放的页面列表中。
     * 页面重用操作基于释放的页面，并按从头到尾的顺序重用页面。
     * @param size, 所需的大小。
     * @param real, 实际分配的大小。
     * @return 此分配的页面索引。
     */
    int ReusePages(int size, int *real);

    /**
     * 申请一些页面。如果有释放的页面，它将首先进行页面重用。
     * 否则，它将分配新页面。
     * @param size, 所需长度的大小。
     * @param real, 实际分配的长度。
     * @return, 此分配的页面索引。
     */
    int AllocPages(int size, int *real);

    /**
     * 获取与此页面关联的已分配大小。
     * @param page, 要查找的页面。
     * @return 与此页面关联的已分配大小。
     */
    int GetAllocPageSize(int page);

    /**
     * 释放指定页面的页面。
     * @param page, 要释放的页面。
     */
    void DeallocPages(int page);

    /**
     * 创建指定名称的部分空间。
     * @param name, 名称的指针。
     * @return 创建的部分空间的指针。
     */
    virtual ISectionSpace *CreateLinearSpace(const char *name);

    /**
     * 删除指定名称的线性空间。
     * @param name, 名称的指针。
     */
    virtual void DropLinearSpace(const char *name);

    /**
     * 查找指定名称的部分。
     * @param name, 名称的指针。
     * @return 部分空间的指针。
     */
    virtual ISectionSpace *FindLinearSpace(const char *name);

    /**
     * 擦除线性空间并清除其部分空间。
     ** @param name, 其名称的指针。
     */
    virtual void EraseLinearSpace(const char *name);

    /**
     * 将源部分空间的数据复制到目标部分空间的末尾。
     * @param dst, 目标部分名称。
     * @param src, 源部分名称。
     * @return, bool, 表示成功与否的布尔值。
     */
    virtual bool AppendLinearSpace(const char *dst, const char *src);
};


#endif //PERSISTENCE_H