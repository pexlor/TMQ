#ifndef LAZY_LINEAR_LIST_H
#define LAZY_LINEAR_LIST_H

#include "Defines.h"
#include "LinearSpace.h"
#include "PageSpace.h"

// 类型定义，用于比较 *p1 和 *p2
typedef int (*Compare)(void *p1, void *p2);

/**
 * 模板类，用于在线性存储空间上读写列表。它是懒惰的读写，除了大小和容量外，所有数据都保存在线性空间上。
 * 懒惰线性列表的描述如下：
 * [size(TMQSize)][T array...]:
 * size 描述了模板对象数组的长度，在 size 之后，是依次存储的所有元素。懒惰线性列表不能扩展或缩小其容量。
 * 应由用户自行扩展或缩小容量。构造 LazyLinearList 时，其容量由页面信息和元素大小确定。
 * 因此，我们可以将懒惰线性列表作为安全的读取器，但要注意添加元素。
 *
 * @tparam T, 模板名称。
 */
template<typename T>
class LazyLinearList {
protected:
    // 指向页面空间的指针。
    IPageSpace *linearSpace;
    // 懒惰线性列表的起始 tmq 地址。
    TMQAddress address;
    // 此列表的容量。
    TMQLSize capacity;
    // 此列表中的元素计数。
    TMQLSize size;

public:
    /**
     * 默认构造函数。
     */
    LazyLinearList() : linearSpace(nullptr), address(ADDRESS_NULL), capacity(0), size(0) {

    }

    /**
     * 使用线性空间、起始地址和容量构造 LazyLinearList。
     * @param linearSpace, 指向页面空间的指针。
     * @param address, 列表的起始地址。
     * @param capacity, 此列表的总容量。
     */
    LazyLinearList(IPageSpace *linearSpace, TMQAddress address, TMQLSize length);

    /*
     * 默认析构函数。
     */
    ~LazyLinearList();

    /**
     * 获取元素计数的方法。
     * @return TMQSize, 元素的计数。
     */
    TMQLSize GetSize();

    /**
     * 获取容量的方法。
     * @return 容量。
     */
    TMQLSize GetCapacity();

    /**
     * 设置方法以修改大小。
     * @param size, 要设置的大小。
     */
    void SetSize(TMQLSize size);

    /**
     * 获取索引处元素的方法。
     * @param index, 元素的索引。
     * @return T, 索引处的元素。
     */
    T Get(long index);

    /**
     * 移除索引处的元素。移除后，索引后的元素将依次移动到它们之前的位置。
     * @param index, 要移除的索引。
     * @return bool, 表示移除是否成功的布尔值。
     */
    bool Remove(long index);

    /**
     * 使用比较函数添加元素。如果比较函数为空，则将元素追加到列表末尾，并将 size 设置为 size + 1。
     * @param t, 元素的值。
     * @param compare, 用于查找正确位置以设置值的比较方法。
     * @return int, 此值的最终位置。
     */
    long Add(const T &t, Compare compare = nullptr);

    /**
     * 使用比较函数查找 t 的位置。
     * @param t, 要查找的值。
     * @param compare, 比较元素的方法。
     * @return int, 表示元素索引的整数值，范围为 [0-size]。
     */
    long FindPosition(const T &t, Compare compare = nullptr);

    /**
     * 在索引处插入值。索引后的元素将移动到它们的下一个位置。
     * @param index, 要插入的索引。
     * @param t, 值。
     * @return bool, 表示插入是否成功的布尔值。
     */
    bool Insert(long index, const T &t);

    /**
     * 清除元素，清除后大小将为零。
     */
    void Clear();

    /**
     * 设置方法以修改索引处的元素。
     * @param index, 要设置的索引。
     * @param t, 新值。
     */
    void Set(long index, const T &t);

    /**
     * 内联方法，用于计算列表索引的页面偏移量。
     * @param index, 列表上的索引。
     * @return, 页面偏移量。
     */
    inline long Offset(long index) {
        if (index >= 0 && index <= size) {
            return OFFSET(address) + 2 * sizeof(TMQSize) + index * sizeof(T);
        }
        return -1;
    }

    /**
     * 内联方法，用于检查列表是否为空。
     * @return 表示列表是否为空的布尔值。
     */
    inline bool Empty() {
        return size == 0;
    }
};

/*
 * 构造 LazyLinearList。初始大小将从线性空间中的地址读取。并且可以通过页面空间的长度和 T 的大小计算容量。
 */
template<typename T>
LazyLinearList<T>::LazyLinearList(IPageSpace *linearSpace, TMQAddress address, TMQLSize length)
    :address(0), size(0), capacity(0) {
    this->linearSpace = linearSpace;
    this->address = address;
    int page = PAGE(address);
    int offset = OFFSET(address);
    // 从线性空间中读取大小值。
    linearSpace->Read(page, offset, &size, sizeof(TMQLSize));
    this->capacity = (length - 2 * sizeof(TMQLSize)) / sizeof(T);
}

/*
 * 构造函数。
 */
template<typename T>
LazyLinearList<T>::~LazyLinearList() {
    size = 0;
    capacity = 0;
}

/*
 * 直接返回大小。
 */
template<typename T>
TMQLSize LazyLinearList<T>::GetSize() {
    return size;
}

/*
 * SetSize 将修改内存中的大小变量，并同时将值保存到页面空间中。
 */
template<typename T>
void LazyLinearList<T>::SetSize(TMQLSize newSize) {
    linearSpace->Write(PAGE(address), OFFSET(address), &newSize, sizeof(TMQSize));
    size = newSize;
}

/*
 * 获取索引处的元素。它将调用页面空间的读取方法获取数据。
 * 如果索引无效，将返回一个空值 'obj'。
 */
template<typename T>
T LazyLinearList<T>::Get(long index) {
    T obj;
    if (index >= 0 && index < GetSize()) {
        // 从线性空间中获取值。
        linearSpace->Read(PAGE(address), Offset(index), &obj, sizeof(T));
    }
    return obj;
}

/*
 * 移除元素，索引后的元素将移动到它们之前的位置。
 * 因此，当有许多元素需要移动时，这将耗费时间。
 */
template<typename T>
bool LazyLinearList<T>::Remove(long index) {
    if (index < 0 || index > GetSize() - 1) {
        return false;
    }
    // 移动索引后的元素。
    for (int i = index; i + 1 < size; ++i) {
        T tmp;
        linearSpace->Read(PAGE(address), Offset(i + 1), &tmp, sizeof(T));
        linearSpace->Write(PAGE(address), Offset(i), &tmp, sizeof(T));
    }
    // 修改大小为 size - 1。
    SetSize(GetSize() - 1);
    return true;
}

/**
 * 在索引处插入元素。索引后的元素将移动到它们的下一个位置。
 * 因此，当有许多元素需要移动时，这将耗费时间。
 * @tparam T, 模板名称。
 * @param index, 要插入的索引。
 * @param t, 值。
 * @return, bool, 表示插入是否成功的布尔值。
 */
template<typename T>
bool LazyLinearList<T>::Insert(long index, const T &t) {
    // 检查并确保索引有效。
    if (index < 0) {
        index = 0;
    }
    // 如果索引 >= GetSize，意味着将元素追加到列表末尾。因此，直接调用 Add 将此元素插入。
    if (index >= GetSize()) {
        return Add(t) >= 0;
    }
    // 发生溢出，忽略插入并返回 false。
    if (GetSize() + 1 > capacity) {
        return false;
    }
    // 移动元素到它们的下一个位置。
    for (int i = GetSize(); i > index; --i) {
        T tmp;
        linearSpace->Read(PAGE(address), Offset(i - 1), &tmp, sizeof(T));
        linearSpace->Write(PAGE(address), Offset(i), &tmp, sizeof(T));
    }
    // 将 t 写入索引位置。
    linearSpace->Write(PAGE(address), Offset(index), (void *) &t, sizeof(T));
    // 修改大小为 size + 1。
    SetSize(GetSize() + 1);
    return true;
}

/**
 * 没有必要处理每个元素，直接将大小设置为零。
 * @tparam T
 */
template<typename T>
void LazyLinearList<T>::Clear() {
    SetSize(0);
}

/*
 * 将新值设置到索引处。
 */
template<typename T>
void LazyLinearList<T>::Set(long index, const T &t) {
    // 检查参数。
    if (index < 0 || index > capacity) {
        return;
    }
    linearSpace->Write(PAGE(address), Offset(index), (void *) &t, sizeof(T));
}

/*
 * 使用比较函数添加元素。如果没有比较函数，将元素追加到列表末尾。
 * 否则，使用 FindPosition 查找此元素的位置，然后将其插入到该位置。
 */
template<typename T>
long LazyLinearList<T>::Add(const T &t, Compare compare) {
    if (GetSize() + 1 > capacity) {
        return -1;
    }
    // 将元素添加到列表末尾。
    if (compare == nullptr) {
        linearSpace->Write(PAGE(address), Offset(GetSize()), (void *) &t, sizeof(T));
        SetSize(GetSize() + 1);
        return GetSize() - 1;
    }
    // 查找此元素的位置。
    int pos = FindPosition(t, compare);
    // 将值插入其位置并返回最终位置。
    if (Insert(pos, t)) {
        return pos;
    }
    return -1;
}

template<typename T>
TMQLSize LazyLinearList<T>::GetCapacity() {
    return capacity;
}

/*
 * 快速排序算法查找值的位置。
 */
template<typename T>
long LazyLinearList<T>::FindPosition(const T &t, Compare compare) {
    if (compare == nullptr) {
        return GetSize();
    }
    long start = 0, end = GetSize() - 1, p = -1;
    int cmp;
    T obj;
    while (start <= end) {
        p = (start + end) / 2;
        linearSpace->Read(PAGE(address), Offset(p), &obj, sizeof(T));
        // 比较两个元素。
        cmp = compare((void *) &t, (void *) &obj);
        if (cmp > 0) {
            start = p + 1;
            p = start;
        } else if (cmp < 0) {
            end = p - 1;
        } else {
            break;
        }
    }
    // 返回最终位置 p。
    return p;
}

#endif //LAZY_LINEAR_LIST_H