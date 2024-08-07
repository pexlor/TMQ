//
//  SectionSpace.cpp
//  SectionSpace
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "SectionSpace.h"

/*
 * Constructor, this will assigned the name, and find the MetaSection associated with this section
 * space.
 */
SectionSpace::SectionSpace(Persistence *persist, const char *sec)
        : SectionAllocator(), name{0}, persist(persist), releaseCount(0) {
    strncpy(this->name, sec, sizeof(this->name));
    // Find or create a meta section.
    metaSection = persist->FindSection(name, true);
}

/*
 * Destructor.
 */
SectionSpace::~SectionSpace() {
    this->persist = nullptr;
}

/*
 * Get the alloc id during each calling of Allocate(), and check whether it has reach to the max
 * value. If allocId reach to the ALLOC_ID_MAX, we will toggle the ResetSpace task. Then invoke
 * Allocate() in its parent.
 */
TMQAddress SectionSpace::Allocate(TMQLSize length) {
    // find last allocId key
    LazyLinearList<SecAlloc> *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (lazyLinearList && lazyLinearList->GetSize() > 0) {
        SecAlloc lastAlloc = lazyLinearList->Get((long) lazyLinearList->GetSize() - 1);
        allocId = SECTION_ALLOC_ID(lastAlloc.secAddress);
    }
    // If allocId reach to the ALLOC_ID_MAX, ResetSpace()
    if (allocId == ALLOC_ID_MAX) {
        allocId = ResetSpace();
    }
    return SectionAllocator::Allocate(length);
}

/**
 * Deallocate a section address. The real Deallocate will occur in its parent, another function of
 * this method is to calculate the release radio and toggle the ResetSpace task.
 * @param address
 */
void SectionSpace::Deallocate(TMQAddress address) {
    SectionAllocator::Deallocate(address);
    TMQSize tmqSize = GetLazyAllocList(LAZY_RESERVE_READ)->GetSize();
    if (tmqSize == 0) {
        persist->EraseLinearSpace(name);
        metaSection.start = -1;
        metaSection.count = 0;
        return;
    }
    if ((float) releaseCount / (float) tmqSize > SPACE_RESET_THRESHOLD) {
        LOG_DEBUG("Deallocate ResetSpace: %d %d", releaseCount, tmqSize);
        ResetSpace();
    }
}

/*
 * Reset and shrink the section space.
 */
unsigned int SectionSpace::ResetSpace() {
    // Get lazy linear list for read.
    auto *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (lazyLinearList == nullptr) {
        return 0;
    }
    // Apply some new pages
    int size = metaSection.count;
    int page = persist->AllocPages(size, &size);
    // Allocate failed.
    LOG_DEBUG("SectionAllocator::ResetSpace %d %d", page, size);
    if (page < 0) {
        return 0;
    }
    // Initialize the new page space.
    persist->GetPageSpace()->Zero(page, 0, size * TMQ_PAGE_SIZE);
    TMQAddress address = ADDRESS(page, 0);
    TMQSize capacity = size * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> newLinearList(persist->GetPageSpace(), address, capacity);
    // Loop for copying the SecAlloc that not released.
    int index = 0;
    while (index < lazyLinearList->GetSize()) {
        SecAlloc metaAlloc = lazyLinearList->Get(index++);
        if (metaAlloc.state != ADDRESS_RELEASE) {
            int secAllocId = SECTION_ALLOC_ID(metaAlloc.secAddress);
            metaAlloc.secAddress = SECTION_ADDRESS(newLinearList.GetSize(), secAllocId);
            newLinearList.Add(metaAlloc);
        }
    }
    releaseCount = 0;
    metaSection.start = page;
    metaSection.count = size;
    // Update to new section.
    persist->UpdateSection(metaSection);
    return newLinearList.GetSize();
}

/*
 * Read data from section space. Firstly, find the SecAlloc, then calculate the tmq address for the
 * real data.
 */
TMQLSize SectionSpace::Read(TMQAddress address, void *buf, TMQLSize length) {
    if (!persist) {
        return -1;
    }
    // Find the SecAlloc associated with the address.
    SecAlloc metaAlloc;
    if (FindAlloc(address, metaAlloc) < 0) {
        return -1;
    }
    // Convert the tmq address to page and its offset.
    auto page = PAGE(metaAlloc.address);
    auto offset = OFFSET(metaAlloc.address);
    return persist->GetPageSpace()->Read(page, offset, buf, length);
}

/*
 * Write data into section space.
 */
TMQLSize SectionSpace::Write(TMQAddress address, void *data, TMQLSize length) {
    if (!persist) {
        return -1;
    }
    // Find the SecAlloc associated with the address.
    SecAlloc metaAlloc;
    if (FindAlloc(address, metaAlloc) < 0) {
        return -1;
    }
    // Convert the tmq address to page and its offset.
    auto page = PAGE(metaAlloc.address);
    auto offset = OFFSET(metaAlloc.address);
    return persist->GetPageSpace()->Write(page, offset, data, length);
}

/*
 * Copy data from the source section address to the destination section address.
 */
void SectionSpace::Copy(TMQAddress dst, TMQAddress src, TMQLSize length) {
    if (!persist) {
        return;
    }
    // Find the source and destination SecAlloc first.
    SecAlloc dstAlloc, srcAlloc;
    if (FindAlloc(dst, dstAlloc) < 0 || FindAlloc(src, srcAlloc) < 0) {
        return;
    }
    // Convert the tmq address to page and its offset.
    auto sp = PAGE(srcAlloc.address);
    auto sf = OFFSET(srcAlloc.address);
    auto dp = PAGE(dstAlloc.address);
    auto df = OFFSET(dstAlloc.address);
    persist->GetPageSpace()->Copy(dp, df, sp, sf, length);
}

/*
 * Set the content to zero.
 */
void SectionSpace::Zero(TMQAddress address, TMQLSize length) {
    if (!persist) {
        return;
    }
    // Find the SecAlloc associated with the address.
    SecAlloc metaAlloc;
    if (FindAlloc(address, metaAlloc) < 0) {
        return;
    }
    // Convert the tmq address to page and its offset.
    auto page = PAGE(metaAlloc.address);
    auto offset = OFFSET(metaAlloc.address);
    persist->GetPageSpace()->Zero(page, offset, length);
}

/*
 * Get the name of this section.
 */
const char *SectionSpace::GetName() {
    return name;
}

/*
 * Get the pointer of the lazy linear list. Three key steps.
 * 1. Find and create the meta section associated with this section space.
 * 2. Resize the section to fulfill the required reserved count.
 * 3. Assigned the local lazy linear list to lazyAllocList and return it.
 */
LazyLinearList<SecAlloc> *SectionSpace::GetLazyAllocList(TMQSize reserve) {
    if (persist == nullptr) {
        return nullptr;
    }
    // If not valid, find and create the meta section associated with this section space.
    if (metaSection.start <= 0) {
        metaSection = persist->FindSection(name, true);
    }
    // Resize the section to fulfill the required reserved count.
    if (!persist->ResizeSection(metaSection, reserve)) {
        return nullptr;
    }
    TMQAddress pageAddress = ADDRESS(metaSection.start, 0);
    TMQSize capacity = metaSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> lazyLinearList(persist->GetPageSpace(), pageAddress, capacity);
    // Assigned the local lazy linear list to lazyAllocList and return it.
    lazyAllocList = lazyLinearList;
    return &lazyAllocList;
}

/*
 * Release the section allocation.
 */
bool SectionSpace::ReleaseAlloc(TMQAddress secAddress) {
    // Find the SecAlloc, if it is not exist, ignore this invoke.
    SecAlloc secAlloc;
    int position = FindAlloc(secAddress, secAlloc);
    if (position < 0) {
        return false;
    }
    // Change its state to ADDRESS_RELEASE.
    secAlloc.state = ADDRESS_RELEASE;
    auto *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (position >= lazyLinearList->GetSize()) {
        return false;
    }
    lazyLinearList->Set(position, secAlloc);
    // Calculate the release radio to toggle the ResetSpace task.
    releaseCount++;
    if ((float) releaseCount / (float) lazyLinearList->GetSize() > SPACE_RESET_THRESHOLD) {
        ResetSpace();
    }
    return true;
}

/*
 * Append a SecAlloc with specified state.
 */
TMQAddress SectionSpace::AppendAlloc(TMQAddress tmqAddress, TMQSize size, TMQLState state) {
    // Get the lazyLinearList for write.
    auto *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_WRITE);
    if (lazyLinearList == nullptr) {
        return false;
    }
    TMQAddress secAddress = SECTION_ADDRESS(lazyLinearList->GetSize(), ++allocId);
    SecAlloc secAlloc(secAddress, tmqAddress, size, state);
    // Save the secAlloc and return the section address.
    if (lazyLinearList->Add(secAlloc) >= 0) {
        return secAddress;
    }
    return ADDRESS_NULL;
}

/*
 * Get the allocated size of the specified page. Invoke GetAllocPageSize in persistence directly.
 */
int SectionSpace::GetAllocPageSize(int page) {
    return persist->GetAllocPageSize(page);
}

/*
 * Deallocate page with the specified page.
 */
void SectionSpace::DeallocPages(int page) {
    persist->DeallocPages(page);
}

/*
 * Allocate pages, delegate this invoke to AllocPages in persistence directly.
 */
int SectionSpace::AllocPages(int size, int *allocSize) {
    return persist->AllocPages(size, allocSize);
}

/*
 * Read the MetaAlloc list in this section space.
 */
bool SectionSpace::GetAllocList(List<MetaAlloc> &allocList) {
    // Get the lazyLinearList for read.
    LazyLinearList<SecAlloc> *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (lazyLinearList == nullptr) {
        return false;
    }
    // Loop lazyLinearList and save MetaAlloc with ADDRESS_ALLOC state to allocList.
    for (int i = 0; i < lazyLinearList->GetSize(); ++i) {
        SecAlloc indexAlloc = lazyLinearList->Get(i);
        if (indexAlloc.state == ADDRESS_ALLOC) {
            MetaAlloc metaAlloc(indexAlloc.secAddress, indexAlloc.size);
            allocList.Add(metaAlloc);
        }
    }
    return true;
}
