//
//  SectionAllocator.cpp
//  SectionAllocator
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//


#include "SectionAllocator.h"

/**
 * Compare function for SecAlloc compare.
 * @param p1, the first SecAlloc pointer to compare.
 * @param p2, the second SecAlloc pointer to compare.
 * @return, a int value for the comparison.
 */
int AllocCompare(void *p1, void *p2) {
    // Force a type conversion
    auto *alloc = (SecAlloc *) p1;
    auto *another = (SecAlloc *) p2;
    // Get the alloc id from section address of *p1.
    int allocId = SECTION_ALLOC_ID(alloc->secAddress);
    // Get the alloc id from section address of *p2.
    int anotherId = SECTION_ALLOC_ID(another->secAddress);
    // comparison by the alloc id.
    if (allocId == anotherId) {
        return 0;
    }
    return allocId > anotherId ? 1 : -1;
}

/*
 * Allocate linear space with required length. Before allocation new linear space, do address reuse
 * first. If address allocation is success, save the tmq address to section space and return the
 * section tmq address.
 */
TMQAddress SectionAllocator::Allocate(TMQLSize length) {
    // Do address reuse, If success, return the address directly.
    TMQAddress secAddress = ReuseAddress(length);
    if (secAddress != ADDRESS_NULL) {
        return secAddress;
    }
    // Address reuse failed, we had to apply to allocate new pages, and divides the pages into two
    // linear space, one is for the return value, and another save to the freed list for the next
    // allocation.
    int allocSize = (int) (length / TMQ_PAGE_SIZE + 1);
    int allocPage = AllocPages(allocSize, &allocSize);
    if (allocPage <= 0) {
        return ADDRESS_NULL;
    }
    TMQAddress allocAddress = ADDRESS(allocPage, 0);
    TMQSize allocLen = allocSize * TMQ_PAGE_SIZE;
    // Save the freed part of the allocated page space.
    SecAlloc leftAlloc(secAddress, allocAddress + length, allocLen - length, ADDRESS_FREE);
    leftAlloc.secAddress = AppendAlloc(leftAlloc.address, leftAlloc.size, leftAlloc.state);
    freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(leftAlloc.address, leftAlloc));
    // Save and return the allocated part.
    SecAlloc metaAlloc(secAddress, allocAddress, length, ADDRESS_ALLOC);
    metaAlloc.secAddress = AppendAlloc(metaAlloc.address, metaAlloc.size, metaAlloc.state);
    return metaAlloc.secAddress;
}

/*
 * Deallocate a section address. When the address is valid, we will find the SecAlloc associated
 * with this section address. Then change the state of the SecAlloc from ADDRESS_ALLOC to
 * ADDRESS_FREE, and put it into freed address map. At last, we will toggle the TryFreePage task.
 */
void SectionAllocator::Deallocate(TMQAddress address) {
    if (address == ADDRESS_NULL) {
        return;
    }
    // Find the SecAlloc associated with this section address
    SecAlloc indexAlloc;
    int allocIndex = FindAlloc(address, indexAlloc);
    // Find fail, ignore.
    if (allocIndex < 0) {
        return;
    }
    auto *lazyLinearList = GetLazyAllocList();
    if (!lazyLinearList) {
        return;
    }
    // Change the state to ADDRESS_FREE, and put this freed tmq address to freedAllocTree.
    indexAlloc.state = ADDRESS_FREE;
    lazyLinearList->Set(allocIndex, indexAlloc);
    freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(indexAlloc.address, indexAlloc));
    // Invoke the page free task.
    TryFreePage(indexAlloc.address);
}

/*
 * The method to do the address reuse. For the linear storage space, we will reuse the address at
 * the beginning ot the linear space always. That is why we use the RbTree to store the tmq address.
 * Because the RbTree can store all freed address with an ascending order. Thus, we can search the
 * freed address at the beginning of RbTree. Upon finding a fulfill linear space, we may reuse
 * address directly when one freed address is fulfill the requirement, or we may divide the long
 * linear space, or merge some small linear space.
 */
TMQAddress SectionAllocator::ReuseAddress(TMQSize size) {
    if (size == 0) {
        return ADDRESS_NULL;
    }
    // Toggle the free address reading task.
    GetFreedAllocList();

    // Iterate the freedAllocTree to calculate a linear space meet the space requirements.
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
    // Check whether the candidate < size, true represents the finding is failed.
    if (candidate < size) {
        return ADDRESS_NULL;
    }
    /**
     *  we have found the reuse address(es), now perform these actions:
     *  1th. release the candidate address(es) from section space
     *  2th. add the alloc address to section space
     *  3th. add the left free address to section space
     *  4th. remove candidate address(es) from freeAllocList
     *  5th. add the left free address to freeAllocList
     *  6th. return the reuse tmq address
     *  Attentions: the steps 1, 2, 3 should be success atomically, since we
     *      have no transaction mechanism now, this will be treated as a known issue.
     */

    // release all of the linear space from the section.
    RbIterator<TMQAddress, SecAlloc> startIterator = indexIterator;
    for (int i = 0; i < count - 1; ++i) {
        startIterator--;
    }
    RbIterator<TMQAddress, SecAlloc> iterator = indexIterator;
    for (int i = 0; i < count; ++i, --iterator) {
        ReleaseAlloc(iterator->value.secAddress);
    }
    // record(append) candidate linear space to section
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
    // append the remaining space
    if (candidate > size) {
        SecAlloc leftAlloc(ADDRESS_NULL, startAlloc.address + size, candidate - size, ADDRESS_FREE);
        leftAlloc.secAddress = AppendAlloc(leftAlloc.address, leftAlloc.size, ADDRESS_FREE);
        freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(leftAlloc.address, leftAlloc));
    }
    return allocAddress;
}

/*
 * Find the SecAlloc associated with the secAddress. When there is no shrink for the section space,
 * the index from the section address is equal to the SecAlloc index in the section space. So, using
 * the index we can find the element, but, if the section address of the SecAlloc is not equal to
 * the secAddress in parameters, it means the section space has been reset. With this situation, we
 * will search the real SecAlloc with the FindPosition method, which is based on quick sort
 * algorithm.
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
    // Check whether the found is success or not.
    if (metaAlloc.secAddress != secAddress) {
        // Find fail, use the FindPosition to search again.
        SecAlloc foundAlloc(secAddress, ADDRESS_NULL, 0, false);
        int pos = lazyLinearList->FindPosition(foundAlloc, AllocCompare);
        if (pos >= 0 && pos < lazyLinearList->GetSize()) {
            metaAlloc = lazyLinearList->Get(pos);
            index = pos;
        }
    }
    // Check whether the find is success or not.
    if (SECTION_ALLOC_ID(metaAlloc.secAddress) != SECTION_ALLOC_ID(secAddress)) {
        // Find fail, return invalid value.
        return -1;
    }
    return index;
}

/*
 * Get freed allocations and put them into freedAllocTree. Every call of this function, it will read
 * limit elements. The max element to read will be TMQ_PAGE_SIZE / sizeof(SecAlloc), which means
 * read content in one page.
 */
void SectionAllocator::GetFreedAllocList() {
    static int cold = 0;
    static TMQSize limit = 0;
    if (cold != -1) {
        auto *lazyLinearList = GetLazyAllocList();
        if (limit == 0) {
            limit = lazyLinearList->GetSize();
        }
        // Calculate the max length to read.
        int i = cold, max = TMQ_PAGE_SIZE / sizeof(SecAlloc);
        for (; i < lazyLinearList->GetSize() && i - cold < max && i < limit; ++i) {
            SecAlloc metaAlloc = lazyLinearList->Get(i);
            if (metaAlloc.state == ADDRESS_FREE) {
                // Read a freed SecAlloc, insert it into freedAllocTree
                freedAllocTree.Insert(Pair<TMQAddress, SecAlloc>(metaAlloc.address, metaAlloc));
            }
        }
        cold = (i - cold == max) ? i : -1;
    }
}

/*
 * What the important action of this method is that it iterates the freedAllocTree and get the freed
 * section address with the same page of the address. Then check whether the page associated with
 * these section address can be freed or not. If true, it will invoke DeallocPages to free the pages.
 */
bool SectionAllocator::TryFreePage(TMQAddress address) {
    if (address == ADDRESS_NULL) {
        return false;
    }
    // Get the page of the address.
    int page = PAGE(address);
    // Get the size associated with this page.
    int size = GetAllocPageSize(page) * TMQ_PAGE_SIZE;
    if (size <= 0) {
        return false;
    }
    // Find the SecAlloc from freedAllocTree
    RbIterator<TMQAddress, SecAlloc> foundIterator = freedAllocTree.Find(address);
    if (foundIterator == freedAllocTree.end()) {
        return false;
    }
    RbIterator<TMQAddress, SecAlloc> left, right, iterator;
    // Iterate left for freed length, and add it to total.
    TMQSize total = 0;
    iterator = foundIterator;
    while (iterator != freedAllocTree.end() && page == PAGE(iterator->value.address)) {
        total += iterator->value.size;
        left = iterator;
        iterator--;
    }
    // Iterate right for freed length, and add it to total.
    right = foundIterator;
    while (++right != freedAllocTree.end() && page == PAGE(right->value.address)) {
        total += right->value.size;
    }
    // If the freed total length is equal to the size allocated by pages, it means that the tmq
    // address allocated from that page all have been freed, so deallocate that page and remove
    // all of the freed address from freedAllocTree and release them with ReleaseAlloc.
    if (total == size) {
        RbIterator<TMQAddress, SecAlloc> it = left;
        do {
            SecAlloc metaAlloc = it->value;
            // Release it from section space.
            ReleaseAlloc(metaAlloc.secAddress);
            // Erase from freedAllocTree
            freedAllocTree.Erase(it++);
        } while (it != right);
        // Deallocate the page and return success.
        DeallocPages(page);
        return true;
    }
    return false;
}
