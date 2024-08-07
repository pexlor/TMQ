#ifndef LINEAR_SPACE_H
#define LINEAR_SPACE_H

#include "Metas.h"
#include "Iterator.h"
#include "List.h"

/**
 * ILinearSpace 是线性空间的接口，它工作在页面空间上，但使用更小的单元对其进行划分，以便进行存储分配。
 * 它是线性存储空间的管理，可以分配和释放指定长度的存储空间。此外，它还提供了从/向线性空间读/写数据的 API，
 * 使用线性空间分配的 tmq 地址。
 */
class ILinearSpace {
public:
    /**
     * 分配指定长度的线性存储空间。
     * @param length, 需要分配的长度。
     * @return 描述已分配线性空间的 tmq 地址。
     */
    virtual TMQAddress Allocate(TMQLSize length) = 0;

    /**
     * 释放由 Allocate() 分配的 tmq 地址。
     * @param address, 要释放的 tmq 地址。
     */
    virtual void Deallocate(TMQAddress address) = 0;

    /**
     * 从地址读取数据并将其保存到结果缓冲区。如果成功，将返回读取的长度，否则返回 -1。
     * @param address, 要读取的地址。
     * @param buf, 保存结果的缓冲区指针。
     * @param length, 要读取的长度。
     * @return long, 已读取的长整型值。
     */
    virtual TMQLSize Read(TMQAddress address, void *buf, TMQLSize length) = 0;

    /**
     * 将数据写入 tmq 地址。
     * @param address, 要写入的地址。
     * @param data, 数据指针。
     * @param length, 数据的长度。
     * @return, long, 已写入的长度。
     */
    virtual TMQLSize Write(TMQAddress address, void *data, TMQLSize length) = 0;

    /**
     * 将源地址的数据复制到线性空间中的目标地址。
     * @param dst, 要复制到的目标地址。
     * @param src, 要复制的源地址。
     * @param length, 要复制的长度。
     */
    virtual void Copy(TMQAddress dst, TMQAddress src, TMQLSize length) = 0;

    /**
     * 将指定长度的 tmq 地址设置为零。
     * @param address, tmq 地址。
     * @param length, 要设置为零的长度。
     */
    virtual void Zero(TMQAddress address, TMQLSize length) = 0;

    /**
     * 虚析构函数。
     */
    virtual ~ILinearSpace() {};
};

/**
 * ISectionSpace 继承自 ILinearSpace，但将整个线性空间划分为多个命名的部分空间。
 * 它是用于保存具有相似属性的对象的功能层。
 */
class ISectionSpace : public ILinearSpace {
public:
    /**
     * 获取部分名称的方法。
     * @return, 指向部分名称的计数指针。
     */
    virtual const char *GetName() = 0;

    /**
     * 获取已分配的 tmq 地址列表。
     * @param allocList, 保存结果的列表。
     * @return, bool, 表示是否有分配的布尔值。
     */
    virtual bool GetAllocList(List<MetaAlloc> &allocList) = 0;

    /**
     * 虚析构函数。
     */
    virtual ~ISectionSpace() {};
};

#endif //LINEAR_SPACE_H