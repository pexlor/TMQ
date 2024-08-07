//
// Metas.h
// Metas
//
// 创建于 2022/5/28.
// 版权所有 (c) 腾讯。保留所有权利。
//

#ifndef TMQ_METAS_H
#define TMQ_METAS_H

#include "Defines.h"
#include "string.h"

/// 常量定义
// 部分名称的长度。由于我们只使用一个页面来保存所有部分名称，部分名称的存储空间有限，我们必须限制部分名称的最大长度。
// 如果 TMQ_PAGE_SIZE 是 4096，那么部分名称的最大计数将是 4096 / 16 = 256。
#define SECTION_NAME_LEN 16
// tmq 页面的字节大小。它应该是系统定义的页面大小的倍数。
#define TMQ_PAGE_SIZE 4096
// 从 tmq 地址获取页面索引
#define PAGE(address) (unsigned int)(address >> 32)
// 从 tmq 地址获取偏移量
#define OFFSET(address) (unsigned int)(address)
// 使用页面和其偏移量制作 tmq 地址。
#define ADDRESS(page, offset) ((TMQAddress)page << 32 | (TMQAddress)offset)
// 无效页面的定义。
#define PAGE_NULL -1
// 无效偏移量的定义。
#define OFFSET_NULL -1
// 无效 tmq 地址的定义。
#define ADDRESS_NULL ADDRESS(PAGE_NULL, OFFSET_NULL)
// TMQAddress 状态
#define ADDRESS_DEFAULT 0
// ADDRESS_ALLOC 表示此地址正在使用中。
#define ADDRESS_ALLOC 1
// ADDRESS_FREE 表示此地址是空闲的，可以在下次分配时重用。
#define ADDRESS_FREE 2
// ADDRESS_RELEASE 表示此地址已释放，正在等待移除。
#define ADDRESS_RELEASE 3
/// 类型定义
// TMQAddress 8 字节计数：[4: pages][4:offset]
typedef unsigned long long TMQAddress;
// 此地址的状态，已分配、释放或释放。参见上面的 TMQAddress 状态。
typedef unsigned char TMQLState;
// 线性空间的 tmq 大小。因为在 64 位机器上的文件可能很大，所以 TMQLSize 定义为无符号长整型。
typedef unsigned long TMQLSize;

/*
 * Section 类。一个部分代表一个连续的线性空间，从页面索引 'start' 开始，并且有 'count' 页面。
 */
class MetaSection {
public:
    // 部分的名称。
    char name[SECTION_NAME_LEN];
    // 此部分的起始索引
    int start;
    // 此部分的页面计数。
    int count;

    /**
     * 默认构造函数。
     */
    MetaSection() : name{0}, start(-1), count(0) {

    }

    /**
     * 使用名称构造 MetaSection。
     * @param name, 名称的常量指针，名称的最大长度限制为 SECTION_NAME_LEN。
     */
    explicit MetaSection(const char *name) : name{0}, start(-1), count(0) {
        strncpy(this->name, name, sizeof(this->name));
    }

    /**
     * 使用名称、起始页面和页面计数构造 MetaSection。
     * @param name, 名称的指针，其长度限制为 SECTION_NAME_LEN。
     * @param s, 起始页面。
     * @param c, 连续页面计数。
     */
    MetaSection(const char *name, int s, int c) : name{0}, start(s), count(c) {
        strncpy(this->name, name, sizeof(this->name));
    }

    /**
     * 重载 '=' 运算符
     * @param section, 原始部分
     * @return, 指向 *this 的引用。
     */
    MetaSection &operator=(const MetaSection &section) {
        this->count = section.count;
        this->start = section.start;
        strncpy(this->name, section.name, sizeof(this->name));
        return *this;
    }
};

/**
 * MetaPage 描述了一个连续的页面空间。它同时描述了占用状态。
 */
class MetaPage {
public:
    // 页面的起始索引。
    unsigned int start;
    // 页面的计数。
    unsigned int count;
    // 表示是否占用的布尔值。如果占用值为 false，则可以重新分配。
    bool occupied;

    /**
     * MetaPage 的默认构造函数。
     */
    MetaPage() : start(0), count(0), occupied(false) {};

    /**
     * 使用起始、计数和占用状态构造 MetaPage。
     * @param start, 页面的起始索引。
     * @param count, 页面的计数。
     * @param occupied, 表示 MetaPage 是否占用的布尔值。
     */
    MetaPage(unsigned int start, unsigned int count, bool occupied = false)
        : start(start), count(count), occupied(occupied) {}
};

/**
 * MetaAlloc 用于描述要分配或已分配的连续线性空间。它具有成员地址和其字节大小。
 */
class MetaAlloc {
public:
    // tmq 地址。
    TMQAddress address;
    // MetaAlloc 的字节大小。
    TMQSize size;

    /**
     * 使用 tmq 地址和其大小构造 MetaAddress。
     * @param address, 分配的地址。
     * @param size, 分配的字节大小。
     */
    MetaAlloc(TMQAddress address, TMQSize size) : address(address), size(size) {}

    /**
     * 默认构造函数。
     */
    MetaAlloc() : address(ADDRESS_NULL), size(0) {}
};

/**
 * 通过部分空间的分配信息。它基于 MetaAlloc，并扩展了一个 secAddress 和此分配的状态。
 * secAddress 用于记录部分空间中的地址，状态用于指示分配是正在使用还是空闲。
 * 状态对于地址重用很有用。
 */
class SecAlloc : public MetaAlloc {
public:
    // 部分空间中的 tmq 地址。
    TMQAddress secAddress;
    // 此分配的状态，ADDRESS_ALLOC, ADDRESS_FREE, ADDRESS_RELEASE。
    TMQLState state;

    SecAlloc() : MetaAlloc(), state(ADDRESS_DEFAULT), secAddress(ADDRESS_NULL) {}

    SecAlloc(TMQAddress sec, TMQAddress tmq, TMQSize size, TMQLState state)
        : MetaAlloc(tmq, size), state(state), secAddress(sec) {}
};


#endif //TMQ_METAS_H