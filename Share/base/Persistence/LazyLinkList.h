//
//  LazyLinkList.h
//  LazyLinkList
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef LAZY_LINK_LIST_H
#define LAZY_LINK_LIST_H

#include <cstdlib>
#include "LazyLinearList.h"
#include "PageSpace.h"
#include "Persistence.h"

/**
 * This is lazy link list, which can record the order of the elements to be added. It holds a lazy
 * linear list to save the list, and expand the LazyNode to record the prev and next element index.
 * Since the insert is so complex and the efficiency is relatively low, so we do not use it any more.
 */
template<typename T>
class LazyNode {
public:
    // value for the node.
    T value;
    // previous index of a node
    int prev;
    // next index of a node
    int next;

    /**
     * Default constructor.
     */
    LazyNode() : next(-1), prev(-1) {}

    /*
     * Constructor with a value.
     */
    LazyNode(const T &t) : value(t), next(-1), prev(-1) {}
};

template<typename T>
class LazyLinkList : public Iterator<T> {
private:
    // header index of the link list.
    int header;
    // tail index of the link list.
    int tail;
    // a pointer the the PageSpace
    IPageSpace *linearSpace;
    // a pointer to the lazy linear list.
    LazyLinearList<LazyNode<T>> *lazyLinearList;
    // a iterator value.
    int iterator;
    // tmq address of the lazy link list.
    TMQAddress address;

public:
    /*
     * Default constructor.
     */
    LazyLinkList() : header(-1), tail(-1), linearSpace(nullptr), lazyLinearList(nullptr) {}

    /**
     * Construct with linear space and tmq address and pages capacity.
     * @param linearSpace, a pointer the page space.
     * @param address , the start address.
     * @param capacity , the capacity of the pages.
     */
    LazyLinkList(IPageSpace *linearSpace, TMQAddress address, TMQSize capacity);

    /**
     * Desctrutor.
     */
    ~LazyLinkList();

    /**
     * Set method for header.
     * @param nh, the new header.
     */
    void SetHeader(int nh);

    /**
     * Set method for the tail.
     * @param nt, the new tail.
     */
    void SetTail(int nt);

    /**
     * Remove a element at index.
     * @param index, index to remove at.
     * @return, success or not.
     */
    bool Remove(long index);

    /**
     * Add an element with compare.
     * @param t, the value
     * @param compare, a pointer to the compare.
     * @return, the position of this element.
     */
    int Add(T &t, Compare compare = nullptr);

    /**
     * Insert an element to the index.
     * @param index, the index to insert to.
     * @param t, the value.
     * @return success or not
     */
    bool Insert(long index, const T &t);

    /**
     * Get method for the element at index.
     * @param index, the index
     * @return the value.
     */
    T Get(long index);

    /**
     * The size of this elements.
     * @return size.
     */
    TMQSize GetSize();

    /**
     * The capacity of this list.
     * @return
     */
    TMQSize GetCapacity();

    /**
     * Modify the element at index.
     * @param index, the index to set.
     * @param alloc, the new value.
     */
    void Set(int index, T alloc);

    /**
     * Remove all elements.
     */
    void Clear();

public:
    // Return a boolean value indicate whether there are elements or not.
    virtual bool HasNext();

    // Get a element and move iterator to next.
    virtual T Next();

    // Remove the current element.
    virtual bool Remove();
};

template<typename T>
LazyLinkList<T>::LazyLinkList(IPageSpace *linearSpace, TMQAddress address, TMQSize capacity)
        :linearSpace(linearSpace), address(address), header(-1), tail(-1), lazyLinearList(nullptr) {
    linearSpace->Read(PAGE(address), OFFSET(address), &header, sizeof(int));
    linearSpace->Read(PAGE(address), OFFSET(address) + sizeof(int), &tail, sizeof(int));
    this->lazyLinearList = new LazyLinearList<LazyNode<T>>(linearSpace, address + sizeof(int) * 2,
                                                           capacity - sizeof(int) * 2);
    // Initialize the header and tail.
    if (this->lazyLinearList->GetSize() == 0) {
        header = -1;
        tail = -1;
    }
    iterator = header;
}

/*
 * Remove element at index.
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
 * Add an element with compare.
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
 * Insert a new value to position index. This will cause the link adjusting.
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
 * Set the header.
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
 * Set the tail
 */
template<typename T>
void LazyLinkList<T>::SetTail(int nt) {
    tail = nt;
    linearSpace->Write(PAGE(address), OFFSET(address) + sizeof(int), &tail, sizeof(int));
}

/*
 * Get value at index.
 */
template<typename T>
T LazyLinkList<T>::Get(long index) {
    LazyNode<T> node = lazyLinearList->Get(index);
    return node.value;
}

/*
 * Destruct the list, release the lazyLinearList.
 */
template<typename T>
LazyLinkList<T>::~LazyLinkList() {
    delete lazyLinearList;
}

/*
 * Get the size.
 */
template<typename T>
TMQSize LazyLinkList<T>::GetSize() {
    return lazyLinearList->GetSize();
}

/*
 * Set the element at index.
 */
template<typename T>
void LazyLinkList<T>::Set(int index, T alloc) {
    LazyNode<T> indexNode = lazyLinearList->Get(index);
    indexNode.value = alloc;
    lazyLinearList->Set(index, indexNode);
}

// Return a boolean value indicate whether there are elements or not.
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
 * Clear all.
 */
template<typename T>
void LazyLinkList<T>::Clear() {
    SetHeader(0);
    SetTail(0);
    lazyLinearList->Clear();
}

/*
 * Get capacity of this list.
 */
template<typename T>
TMQSize LazyLinkList<T>::GetCapacity() {
    return lazyLinearList->GetCapacity();
}


#endif //LAZY_LINK_LIST_H
