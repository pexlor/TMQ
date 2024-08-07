#include "SectionAllocator.h"

/**
 * SecAlloc 比较函数的比较函数。
 * @param p1, 第一个 SecAlloc 指针进行比较。
 * @param p2, 第二个 SecAlloc 指针进行比较。
 * @return, 比较的整数值。
 */
int AllocCompare(void *p1, void *p2) {
    // 强制类型转换
    auto *alloc = (SecAlloc *) p1;
    auto *another = (SecAlloc *) p2;
    // 从 *p1 的部分地址获取分配 id。
    int allocId = SECTION_ALLOC_ID(alloc->secAddress);
    // 从 *p2 的部分地址获取分配 id。
    int anotherId = SECTION_ALLOC_ID(another->secAddress);
    // 通过分配 id 进行比较。
    if (allocId == anotherId) {
        return 0;
    }
    return allocId > anotherId ? 1 : -1;
}

/*
 * 分配所需长度的线性空间。在分配新的线性空间之前，首先进行地址重用。
 * 如果地址分配成功，将 tmq 地址保存到部分空间并返回部分 tmq 地址。
 */
TMQAddress SectionAllocator::Allocate(TMQLSize length) {
    // 进行地址重用，如果成功，直接返回地址。
    TMQAddress secAddress = ReuseAddress(length);
    if (secAddress != ADDRESS_NULL) {
        return secAddress;
    }
    // 地址重用失败，我们必须申请新的页面，并将页面分为两个线性空间，
    // 一个是返回值，另一个保存到下一个分配的释放列表中。
    int allocSize = (int) (length / TMQ_PAGE_SIZE + 1);
    int allocPage = AllocPages(allocSize, &allocSize);
    if (allocPage <= 0) {
        return ADDRESS_NULL;
    }
    TMQAddress allocAddress = ADDRESS(allocPage, 0);
    TMQSize allocLen = allocSize * TMQ_PAGE_SIZE;
    // 保存分配页面空间的释放部分。
    SecAlloc leftAlloc(secAddress, allocAddress + length, allocLen - length, ADDRESS_FREE);
    leftAlloc.secAddress = AppendAlloc(leftAlloc.address, leftAlloc.size, leftAlloc.state);
    freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(leftAlloc.address, leftAlloc));
    // 保存并返回分配的部分。
    SecAlloc metaAlloc(secAddress, allocAddress, length, ADDRESS_ALLOC);
    metaAlloc.secAddress = AppendAlloc(metaAlloc.address, metaAlloc.size, metaAlloc.state);
    return metaAlloc.secAddress;
}

/*
 * 释放部分地址。当地址有效时，我们将找到与此部分地址关联的 SecAlloc。
 * 然后将 SecAlloc 的状态从 ADDRESS_ALLOC 更改为 ADDRESS_FREE，并将其放入释放地址映射中。
 * 最后，我们将切换 TryFreePage 任务。
 */
void SectionAllocator::Deallocate(TMQAddress address) {
    if (address == ADDRESS_NULL) {
        return;
    }
    // 查找与此部分地址关联的 SecAlloc
    SecAlloc indexAlloc;
    int allocIndex = FindAlloc(address, indexAlloc);
    // 查找失败，忽略。
    if (allocIndex < 0) {
        return;
    }
    auto *lazyLinearList = GetLazyAllocList();
    if (!lazyLinearList) {
        return;
    }
    // 将状态更改为 ADDRESS_FREE，并将此释放的 tmq 地址放入 freedAllocTree。
    indexAlloc.state = ADDRESS_FREE;
    lazyLinearList->Set(allocIndex, indexAlloc);
    freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(indexAlloc.address, indexAlloc));
    // 调用页面释放任务。
    TryFreePage(indexAlloc.address);
}

/*
 * 进行地址重用的方法。对于线性存储空间，我们总是重用线性空间开头的地址。
 * 这就是为什么我们使用 RbTree 存储 tmq 地址的原因。
 * 因为 RbTree 可以按升序存储所有释放的地址。因此，我们可以搜索 RbTree 开头的释放地址。
 * 找到一个满足空间要求的线性空间后，我们可以直接重用地址，
 * 或者我们可以分割长的线性空间，或者合并一些小的线性空间。
 */
TMQAddress SectionAllocator::ReuseAddress(TMQSize size) {
    if (size == 0) {
        return ADDRESS_NULL;
    }
    // 切换释放地址读取任务。
    GetFreedAllocList();

    // 遍历 freedAllocTree 以计算满足空间要求的线性空间。
    int page = -1, count = 0;
    TMQSize candidate = 0;
    RbIterator<TMQAddress, SecAlloc> indexIterator = freedAllocTree.begin();
    while (indexIterator != freedAllocTree.end()) {
        SecAlloc metaAlloc = indexIterator->value;
        if (page == PAGE(metaAlloc.address)) {
            candidate += metaAlloc.size;
            count += 1;
        } else {
            candidate = metaAlloc.size;
            count = 1;
            page = PAGE(metaAlloc.address);
        }
        if (candidate >= size) {
            break;
        }
        indexIterator++;
    }
    // 检查候选者是否 < size，true 表示查找失败。
    if (candidate < size) {
        return ADDRESS_NULL;
    }
    /**
     * 我们找到了重用的地址（es），现在执行以下操作：
     * 1th. 从部分空间释放候选地址（es）
     * 2th. 将分配地址添加到部分空间
     * 3th. 将左侧释放地址添加到部分空间
     * 4th. 从 freeAllocList 中删除候选地址（es）
     * 5th. 将左侧释放地址添加到 freeAllocList
     * 6th. 返回重用的 tmq 地址
     * 注意事项：步骤 1、2、3 应该原子性地成功，由于我们现在没有事务机制，这将被视为已知问题。
     */

    // 从部分释放所有线性空间。
    RbIterator<TMQAddress, SecAlloc> startIterator = indexIterator;
    for (int i = 0; i < count - 1; ++i) {
        startIterator--;
    }
    RbIterator<TMQAddress, SecAlloc> iterator = indexIterator;
    for (int i = 0; i < count; ++i, --iterator) {
        ReleaseAlloc(iterator->value.secAddress);
    }
    // 记录（附加）候选线性空间到部分
    SecAlloc startAlloc = startIterator->value;
    TMQAddress allocAddress = AppendAlloc(startAlloc.address, size, ADDRESS_ALLOC);
    if (allocAddress == ADDRESS_NULL) {
        return ADDRESS_NULL;
    }

    RbIterator<TMQAddress, SecAlloc> endIterator = indexIterator;
    endIterator++;
    while (startIterator != endIterator) {
        freedAllocTree.Erase(startIterator++);
    }
    // 附加剩余空间
    if (candidate > size) {
        SecAlloc leftAlloc(ADDRESS_NULL, startAlloc.address + size, candidate - size, ADDRESS_FREE);
        leftAlloc.secAddress = AppendAlloc(leftAlloc.address, leftAlloc.size, ADDRESS_FREE);
        freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(leftAlloc.address, leftAlloc));
    }
    return allocAddress;
}

/*
 * 查找与 secAddress 关联的 SecAlloc。当部分空间没有收缩时，
 * 部分地址的索引等于部分空间中 SecAlloc 的索引。因此，使用索引我们可以找到元素，
 * 但是，如果 SecAlloc 的部分地址不等于参数中的 secAddress，这意味着部分空间已被重置。
 * 在这种情况下，我们将使用基于快速排序算法的 FindPosition 方法再次搜索真正的 SecAlloc。
 */
int SectionAllocator::FindAlloc(TMQAddress secAddress, SecAlloc &metaAlloc) {
    if (secAddress == ADDRESS_NULL) {
        return -1;
    }
    auto *lazyLinearList = GetLazyAllocList();
    if (lazyLinearList == nullptr) {
        return -1;
    }
    int index = SECTION_INDEX(secAddress);
    metaAlloc = lazyLinearList->Get(index);
    // 检查找到的是否成功。
    if (metaAlloc.secAddress != secAddress) {
        // 查找失败，使用 FindPosition 再次搜索。
        SecAlloc foundAlloc(secAddress, ADDRESS_NULL, 0, false);
        int pos = lazyLinearList->FindPosition(foundAlloc, AllocCompare);
        if (pos >= 0 && pos < lazyLinearList->GetSize()) {
            metaAlloc = lazyLinearList->Get(pos);
            index = pos;
        }
    }
    // 检查找找是否成功。
    if (SECTION_ALLOC_ID(metaAlloc.secAddress) != SECTION_ALLOC_ID(secAddress)) {
        // 查找失败，返回无效值。
        return -1;
    }
    return index;
}

/*
 * 获取释放的分配并将它们放入 freedAllocTree。每次调用此函数时，它都会读取限制元素。
 * 最大读取元素数为 TMQ_PAGE_SIZE / sizeof(SecAlloc)，这意味着在一个页面中读取内容。
 */
void SectionAllocator::GetFreedAllocList() {
    static int cold = 0;
    static TMQSize limit = 0;
    if (cold != -1) {
        auto *lazyLinearList = GetLazyAllocList();
        if (limit == 0) {
            limit = lazyLinearList->GetSize();
        }
        // 计算最大读取长度。
        int i = cold, max = TMQ_PAGE_SIZE / sizeof(SecAlloc);
        for (; i < lazyLinearList->GetSize() && i - cold < max && i < limit; ++i) {
            SecAlloc metaAlloc = lazyLinearList->Get(i);
            if (metaAlloc.state == ADDRESS_FREE) {
                // 读取一个释放的 SecAlloc，将其插入到 freedAllocTree
                freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(metaAlloc.address, metaAlloc));
            }
        }
        cold = (i - cold == max) ? i : -1;
    }
}

/*
 * 此方法的重要动作是遍历 freedAllocTree 并获取与地址相同页面的释放部分地址。
 * 然后检查与这些部分地址关联的页面是否可以释放。如果为真，它将调用 DeallocPages 释放页面。
 */
bool SectionAllocator::TryFreePage(TMQAddress address) {
    if (address == ADDRESS_NULL) {
        return false;
    }
    // 获取地址的页面。
    int page = PAGE(address);
    // 获取与此页面关联的大小。
    int size = GetAllocPageSize(page) * TMQ_PAGE_SIZE;
    if (size <= 0) {
        return false;
    }
    // 从 freedAllocTree 中找到 SecAlloc
    RbIterator<TMQAddress, SecAlloc> foundIterator = freedAllocTree.Find(address);
    if (foundIterator == freedAllocTree.end()) {
        return false;
    }
    RbIterator<TMQAddress, SecAlloc> left, right, iterator;
    // 遍历左侧以释放长度，并将其添加到总计。
    TMQSize total = 0;
    iterator = foundIterator;
    while (iterator != freedAllocTree.end() && page == PAGE(iterator->value.address)) {
        total += iterator->value.size;
        left = iterator;
        iterator--;
    }
    // 遍历右侧以释放长度，并将其添加到总计。
    right = foundIterator;
    while (++right != freedAllocTree.end() && page == PAGE(right->value.address)) {
        total += right->value.size;
    }
    // 如果释放的总长度等于页面分配的大小，这意味着从该页面分配的所有 tmq
    // 地址都已被释放，因此释放该页面并从 freedAllocTree 中删除所有释放的地址并释放它们。
    if (total == size) {
        RbIterator<TMQAddress, SecAlloc> it = left;
        do {
            SecAlloc metaAlloc = it->value;
            // 从部分空间释放它。
            ReleaseAlloc(metaAlloc.secAddress);
            // 从 freedAllocTree 中擦除
            freedAllocTree.Erase(it++);
        } while (it != right);
        // 释放页面并返回成功。
        DeallocPages(page);
        return true;
    }
    return false;
}