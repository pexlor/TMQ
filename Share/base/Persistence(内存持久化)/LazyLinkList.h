//
// LazyLinkList.h
// LazyLinkList
//
// 创建于 2022/5/28.
// 版权 (c) 腾讯。保留所有权利。
//

#ifndef LAZY_LINK_LIST_H
#define LAZY_LINK_LIST_H

#include <cstdlib>
#include "LazyLinearList.h"
#include "PageSpace.h"
#include "Persistence.h"

/**
 * 这是一个懒惰链表，可以记录要添加元素的顺序。它持有一个懒惰线性列表来保存列表，并扩展 LazyNode 以记录前一个和下一个元素的索引。
 * 由于插入操作非常复杂且效率相对较低，因此我们不再使用它。
 */
template<typename T>
class LazyNode {
public:
    // 节点的值。
    T value;
    // 节点的前一个索引
    int prev;
    // 节点的下一个索引
    int next;

    /**
     * 默认构造函数。
     */
    LazyNode() : next(-1), prev(-1) {}

    /*
     * 带值的构造函数。
     */
    LazyNode(const T &t) : value(t), next(-1), prev(-1) {}
};

template<typename T>
class LazyLinkList : public Iterator<T> {
private:
    // 链表的头索引。
    int header;
    // 链表的尾索引。
    int tail;
    // 指向 PageSpace 的指针
    IPageSpace *linearSpace;
    // 指向懒惰线性列表的指针。
    LazyLinearList<LazyNode<T>> *lazyLinearList;
    // 迭代器值。
    int iterator;
    // 懒惰链表的 tmq 地址。
    TMQAddress address;

public:
    /*
     * 默认构造函数。
     */
    LazyLinkList() : header(-1), tail(-1), linearSpace(nullptr), lazyLinearList(nullptr) {}

    /**
     * 使用线性空间、tmq 地址和页面容量构造。
     * @param linearSpace, 指向页面空间的指针。
     * @param address , 起始地址。
     * @param capacity , 页面的容量。
     */
    LazyLinkList(IPageSpace *linearSpace, TMQAddress address, TMQSize capacity);

    /**
     * 析构函数。
     */
    ~LazyLinkList();

    /**
     * 设置头的方法。
     * @param nh, 新的头。
     */
    void SetHeader(int nh);

    /**
     * 设置尾的方法。
     * @param nt, 新的尾。
     */
    void SetTail(int nt);

    /**
     * 在索引处移除元素。
     * @param index, 要移除的索引。
     * @return, 成功与否。
     */
    bool Remove(long index);

    /**
     * 使用比较添加元素。
     * @param t, 值
     * @param compare, 指向比较的指针。
     * @return, 该元素的索引位置。
     */
    int Add(T &t, Compare compare = nullptr);

    /**
     * 将值插入到索引处。
     * @param index, 要插入的索引。
     * @param t, 值。
     * @return 成功与否
     */
    bool Insert(long index, const T &t);

    /**
     * 获取索引处的元素。
     * @param index, 索引
     * @return 值。
     */
    T Get(long index);

    /**
     * 此元素的大小。
     * @return 大小。
     */
    TMQSize GetSize();

    /**
     * 该列表的容量。
     * @return
     */
    TMQSize GetCapacity();

    /**
     * 修改索引处的元素。
     * @param index, 要设置的索引。
     * @param alloc, 新值。
     */
    void Set(int index, T alloc);

    /**
     * 移除所有元素。
     */
    void Clear();

public:
    // 返回一个布尔值表示是否有元素。
    virtual bool HasNext();

    // 获取一个元素并将迭代器移动到下一个。
    virtual T Next();

    // 移除当前元素。
    virtual bool Remove();
};

template<typename T>
LazyLinkList<T>::LazyLinkList(IPageSpace *linearSpace, TMQAddress address, TMQSize capacity)
    :linearSpace(linearSpace), address(address), header(-1), tail(-1), lazyLinearList(nullptr) {
    linearSpace->Read(PAGE(address), OFFSET(address), &header, sizeof(int));
    linearSpace->Read(PAGE(address), OFFSET(address) + sizeof(int), &tail, sizeof(int));
    this->lazyLinearList = new LazyLinearList<LazyNode<T>>(linearSpace, address + sizeof(int) * 2,
                                                      capacity - sizeof(int) * 2);
    // 初始化头和尾。
    if (this->lazyLinearList->GetSize() == 0) {
        header = -1;
        tail = -1;
    }
    iterator = header;
}

/*
 * 移除索引处的元素。
 */
template<typename T>
bool LazyLinkList<T>::Remove(long index) {
    LazyNode<T> indexNode = Get(index);
    int prev = indexNode.prev;
    int next = indexNode.next;
    if (prev >= 0) {
        LazyNode<T> preNode = lazyLinearList->Get(prev);
        preNode.next = next;
        lazyLinearList->Set(prev, preNode);
    } else {
        SetHeader(next);
    }
    if (next >= 0) {
        LazyNode<T> nextNode = lazyLinearList->Get(next);
        nextNode.prev = prev;
        lazyLinearList->Set(next, nextNode);
    } else {
        SetTail(prev);
    }
    return lazyLinearList->Remove(index);
}

/*
 * 使用比较添加元素。
 */
template<typename T>
int LazyLinkList<T>::Add(T &t, Compare compare) {
    LazyNode<T> lazyNode(t);
    int nt = lazyLinearList->FindPosition(t, compare);
    if (nt < 0) {
        nt = 0;
    }
    if (nt > lazyLinearList->GetSize()) {
        nt = lazyLinearList->GetSize();
    }
    Insert(nt, t);
    return nt;
}

/*
 * 将新值插入到索引位置。这将导致链接调整。
 */
template<typename T>
bool LazyLinkList<T>::Insert(long index, const T &t) {
    if (index < 0) {
        index = 0;
    }
    if (index > lazyLinearList->GetSize()) {
        index = lazyLinearList->GetSize();
    }
    for (int i = index; i < lazyLinearList->GetSize(); ++i) {
        LazyNode<T> indexNode = lazyLinearList->Get(i);
        if (indexNode.prev < index) {
            LazyNode<T> prevNode = lazyLinearList->Get(indexNode.prev);
            prevNode.next += 1;
            lazyLinearList->Set(indexNode.prev, prevNode);
        }
        if (indexNode.next < index) {
            LazyNode<T> nextNode = lazyLinearList->Get(indexNode.next);
            nextNode.prev += 1;
            lazyLinearList->Set(indexNode.next, nextNode);
        }
        if (indexNode.prev >= index) {
            indexNode.prev += 1;
        }
        if (indexNode.next >= index) {
            indexNode.next += 1;
        }
        lazyLinearList->Set(i, indexNode);
    }
    if (header == -1 || header >= index) {
        SetHeader(header + 1);
    }
    if (tail >= 0) {
        LazyNode<T> tailNode = lazyLinearList->Get(tail);
        tailNode.next = index;
        lazyLinearList->Set(tail, tailNode);
    }
    if (tail >= index) {
        SetTail(tail + 1);
    }
    LazyNode<T> lazyNode(t);
    lazyNode.prev = tail;
    lazyNode.next = -1;
    for (int i = 0; i < lazyLinearList->GetSize(); ++i) {
        LazyNode<T> node = lazyLinearList->Get(i);
    }
    lazyLinearList->Insert(index, lazyNode);
    SetTail(index);
    return true;
}

/*
 * 设置头。
 */
template<typename T>
void LazyLinkList<T>::SetHeader(int nh) {
    if (header == iterator) {
        iterator = nh;
    }
    header = nh;
    linearSpace->Write(PAGE(address), OFFSET(address), &header, sizeof(int));
}

/*
 * 设置尾
 */
template<typename T>
void LazyLinkList<T>::SetTail(int nt) {
    tail = nt;
    linearSpace->Write(PAGE(address), OFFSET(address) + sizeof(int), &tail, sizeof(int));
}

/*
 * 获取索引处的值。
 */
template<typename T>
T LazyLinkList<T>::Get(long index) {
    LazyNode<T> node = lazyLinearList->Get(index);
    return node.value;
}

/*
 * 析构列表，释放 lazyLinearList。
 */
template<typename T>
LazyLinkList<T>::~LazyLinkList() {
    delete lazyLinearList;
}

/*
 * 获取大小。
 */
template<typename T>
TMQSize LazyLinkList<T>::GetSize() {
    return lazyLinearList->GetSize();
}

/*
 * 设置索引处的元素。
 */
template<typename T>
void LazyLinkList<T>::Set(int index, T alloc) {
    LazyNode<T> indexNode = lazyLinearList->Get(index);
    indexNode.value = alloc;
    lazyLinearList->Set(index, indexNode);
}

// 返回一个布尔值表示是否有元素。
template<typename T>
bool LazyLinkList<T>::HasNext() {
    return tail >= 0 && iterator != -1;
}

template<typename T>
T LazyLinkList<T>::Next() {
    if (iterator == -1) {
        return T();
    }
    LazyNode<T> iteratorNode = lazyLinearList->Get(iterator);
    iterator = iteratorNode.next;
    return iteratorNode.value;
}

template<typename T>
bool LazyLinkList<T>::Remove() {
    LazyNode<T> iteratorNode = lazyLinearList->Get(iterator);
    bool suc = lazyLinearList->Remove(iterator);
    iterator = iteratorNode.next;
    return suc;
}

/*
 * 清除所有。
 */
template<typename T>
void LazyLinkList<T>::Clear() {
    SetHeader(0);
    SetTail(0);
    lazyLinearList->Clear();
}

/*
 * 获取此列表的容量。
 */
template<typename T>
TMQSize LazyLinkList<T>::GetCapacity() {
    return lazyLinearList->GetCapacity();
}

#endif //LAZY_LINK_LIST_H