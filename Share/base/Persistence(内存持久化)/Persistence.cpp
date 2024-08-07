#include "string.h"
#include "Defines.h"
#include "Persistence.h"
#include "LazyLinearList.h"
#include "LazyLinkList.h"
#include "SectionSpace.h"

/// Const definitions
#define PAGE_UNFIXED                -1
#define SECTIONS_PAGE_START         0
#define SECTIONS_PAGE_OFFSET        0
#define SECTIONS_PAGE_COUNT         1
#define SECTIONS_ADDRESS            ADDRESS(SECTIONS_PAGE_START, SECTIONS_PAGE_OFFSET)
#define ALLOC_FACTOR                0.2
#define ALLOC_OVERFLOW              2

/*
 * Construct a persistence. Two important steps:
 * 1. Create a lazySectionList on the page space.
 * 2. Check whether there are data in the page space. if no any data, require the first page to
 *  save the meta sections and add the SECTION_ALLOC MetaSection to the lazySectionList.
 */
/*

/*构造一个持久性对象。两个重要步骤：
    在页面空间上创建一个lazySectionList。
    检查页面空间中是否有数据。如果没有任何数据，请求第一页来保存元段并将SECTION_ALLOC MetaSection添加到lazySectionList中。 
*/ 
Persistence::Persistence(IPageSpace *pageSpace) : pageSpace(pageSpace), sectionSpaces{} {
    lazySectionList = new LazyLinearList<MetaSection>(pageSpace,
                                                      SECTIONS_ADDRESS, TMQ_PAGE_SIZE);
    if (lazySectionList->Empty()) {
        pageSpace->Allocate(SECTIONS_PAGE_START, SECTIONS_PAGE_COUNT);
        lazySectionList->Add(MetaSection(SECTION_ALLOC));
    }
}

/*
 * Destructor, delete lazySectionList.
 */
/*
析构函数，删除lazySectionList。 */
Persistence::~Persistence() {
    delete lazySectionList;
}

/*
 * Compare function for the MetaPage.
 */
/*对MetaPage的比较函数。*/
int PageCompare(void *p1, void *p2) {
    auto *alloc = (MetaPage *) p1;
    auto *another = (MetaPage *) p2;
    // Check the page start, if it is equal, return 0.
    if (alloc->start == another->start) {
        return 0;
    }
    return alloc->start > another->start ? 1 : -1;
}

/*
 * Create a linear space with the specified name.
 */
/*
创建一个指定名称的线性空间。 */
ISectionSpace *Persistence::CreateLinearSpace(const char *name) {
    // Check whether it is exist or not.
    auto *sectionSpace = FindLinearSpace(name);
    if (!sectionSpace) {
        // There is no section space specified by name, create it.
        MetaSection section = FindSection(name);
        if (section.start > 0) {
            sectionSpace = new SectionSpace(this, name);
            sectionSpaces.Add(sectionSpace);
        }
    }
    return sectionSpace;
}

/*
 * Drop a linear space, this will erase the linear space, be careful with this call.
 */
/*删除一个线性空间，这将删除线性空间，调用此函数时要小心。 */ 
void Persistence::DropLinearSpace(const char *name) {
    // Erase the section space.
    EraseLinearSpace(name);
    // Remove the specified section space.
    for (int i = 0; i < sectionSpaces.Size(); ++i) {
        auto *indexLinearSpace = (SectionSpace *) sectionSpaces.Get(i);
        if (strcmp(indexLinearSpace->GetName(), name) == 0) {
            // Remove and delete the section space.
            sectionSpaces.Remove(i);
            delete indexLinearSpace;
            break;
        }
    }
}

/*
 * Erase a linear space, erase the meta section too.
 */

/*删除一个线性空间，也删除元区段。 */ 
void Persistence::EraseLinearSpace(const char *name) {
    // Check parameters.
    if (!name || strcmp(SECTION_ALLOC, name) == 0) {
        return;
    }
    // Erase the section.
    EraseSection(name);
}

/*
 * Find a linear space with the specified section name.
 */
/*使用指定的区段名称查找线性空间。 */ 
ISectionSpace *Persistence::FindLinearSpace(const char *name) {
    SectionSpace *sectionLinearSpace = nullptr;
    // Loop to find the section space.
    for (int i = 0; i < sectionSpaces.Size(); ++i) {
        auto *indexLinearSpace = (SectionSpace *) (sectionSpaces.Get(i));
        if (strcmp(indexLinearSpace->GetName(), name) == 0) {
            sectionLinearSpace = indexLinearSpace;
            break;
        }
    }
    return sectionLinearSpace;
}

/*
 * Return the page space.
 */
/*返回页面空间。 */
IPageSpace *Persistence::GetPageSpace() {
    return pageSpace;
}

/*
 * Implementation for page allocations. Key steps:
 * 1. Do page reuse first, if success, return the allocated page.
 * 2. Allocate new pages from page space.
 * 3. Save the allocated pages to the section SECTION_ALLOC.
 * Before add new allocated pages to the section SECTION_ALLOC, it must ensure that the
 * SECTION_ALLOC section is not overflow. If it is overflow, move the section to new pages.
 */
/*
页面分配的实现。关键步骤：
首先进行页面重用，如果成功，返回分配的页面。
从页面空间分配新页面。
将分配的页面保存到SECTION_ALLOC区段中。
在将新分配的页面添加到SECTION_ALLOC区段之前，必须确保
SECTION_ALLOC区段没有溢出。如果溢出，将该区段移动到新页面。 */
int Persistence::AllocPages(int size, int *real) {
    if (size <= 0) {
        return PAGE_NULL;
    }
    int reuse = ReusePages(size, real);
    MetaPage allocPage(reuse, *real, true);
    // find alloc section
    MetaPage sectionPage;
    int secIndex = -1;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        MetaSection indexSection = lazySectionList->Get(i);
        if (strcmp(indexSection.name, SECTION_ALLOC) == 0) {
            secIndex = i;
            break;
        }
    }
    if (secIndex < 0) {
        return PAGE_NULL;
    }
    MetaSection allocSection = lazySectionList->Get(secIndex);
    int oldSectionPage = PAGE_NULL;
    // Check whether the allocSection is overflow, if it is overflow, move it to new section.
    if (allocSection.start == PAGE_NULL || Overflow(allocSection, ALLOC_OVERFLOW)) {
        int newCount = (int) (allocSection.count * (1 + ALLOC_FACTOR) + 1);
        int newIndex = pageSpace->Allocate(PAGE_UNFIXED, newCount);
        if (!MoveSection(allocSection, newIndex, newCount)) {
            DeallocPages(newIndex);
            return PAGE_NULL;
        }
        oldSectionPage = allocSection.start;
        allocSection.start = newIndex;
        allocSection.count = newCount;
        sectionPage.start = allocSection.start;
        sectionPage.count = allocSection.count;
        sectionPage.occupied = true;
    }
    // alloc from page space
    if (allocPage.start == PAGE_NULL || allocPage.count <= 0) {
        allocPage.start = pageSpace->Allocate(PAGE_UNFIXED, size);
        allocPage.count = size;
    }
    // save to alloc space
    TMQAddress allocAddress = ADDRESS(allocSection.start, 0);
    TMQSize allocCapacity = allocSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<MetaPage> lazyPageList(pageSpace, allocAddress, allocCapacity);
    if (sectionPage.start != PAGE_NULL && sectionPage.count > 0) {
        lazyPageList.Add(sectionPage, PageCompare);
    }
    if (allocPage.start != PAGE_NULL && allocPage.count > 0) {
        lazyPageList.Add(allocPage, PageCompare);
    }
    *real = allocPage.count;
    // Deallocate old section page.
    if (oldSectionPage > 0) {
        DeallocPages(oldSectionPage);
    }
    return (int) allocPage.start;
}

/*
 * Reuse pages. This function will search the freed pages from beginning to end. There a two scenes
 * can used a page reuse. One is that there is a freed page whose size is fulfilled the required
 * size, so it is success. Another is that there are freed pages but all of them are continuous, we
 * combine the them into new page as return.
 */
/*
重用页面。这个函数会从头到尾搜索已释放的页面。有两种情况可以重用页面。一种是有一个已释放的页面，其大小满足所需的
大小，所以它是成功的。另一种是有已释放的页面，但它们都是连续的，我们将它们合并为新页面返回。 */

int Persistence::ReusePages(int size, int *real) {
    // Find the SECTION_ALLOC meta section.
    int sectionIndex = -1;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        if (strcmp(lazySectionList->Get(i).name, SECTION_ALLOC) == 0) {
            sectionIndex = i;
            break;
        }
    }
    // Check whether the SECTION_ALLOC is exist or not.
    if (sectionIndex >= 0) {
        MetaSection pageSection = lazySectionList->Get(sectionIndex);
        TMQAddress pageAddress = ADDRESS(pageSection.start, 0);
        TMQSize capacity = pageSection.count * TMQ_PAGE_SIZE;
        LazyLinearList<MetaPage> lazyPageList(pageSpace, pageAddress, capacity);
        // Algorithm for calculating the freed pages that can meet the required size.
        unsigned int last = PAGE_NULL;
        unsigned int combLen = 0;
        int combIndex = -1;
        for (int i = 0; i < lazyPageList.GetSize(); ++i) {
            MetaPage indexPage = lazyPageList.Get(i);
            if (indexPage.occupied) {
                last = PAGE_NULL;
                combLen = 0;
                combIndex = -1;
                continue;
            }
            if (last == indexPage.start && !indexPage.occupied) {
                combLen += indexPage.count;
            } else {
                last = indexPage.start;
                combLen = indexPage.count;
                combIndex = i;
            }
            // The length is fulfilled the required size, break the loop.
            if (combLen >= size) {
                break;
            }
            last = indexPage.start + indexPage.count;
        }
        // Find success, do page reuse.
        if (combIndex >= 0 && combLen >= size) {
            LOG_DEBUG("Reuse pages, page:%d ,combine count:%d, combine index:%d, Size:%d",
                      last, combLen, combIndex, size);
            MetaPage combPage = lazyPageList.Get(combIndex);
            unsigned int page = combPage.start;
            // The left pages has been combined, Release them.
            while (combPage.count > 0 && combPage.start <= last) {
                lazyPageList.Remove(combIndex);
                combPage = lazyPageList.Get(combIndex);
            }
            // Assigned the new real length.
            *real = combLen;
            // Return the page index.
            return page;
        }
    }
    *real = 0;
    return PAGE_NULL;
}

/*
 * Deallocate a page. Every deallocating, we will check the page is at the end of the PageSpace, if
 * the page is at the end, we can deallocate that page from PageSpace.
 */
/*
释放一个页面。每次释放时，我们将检查页面是否位于PageSpace的末尾，如果
页面在末尾，我们可以从PageSpace中释放该页面。 */
void Persistence::DeallocPages(int page) {
    if (page <= 0) {
        return;
    }
    // Find MetaPage associated with this page from the allocated pages.
    MetaSection allocPageSection = FindSection(SECTION_ALLOC, false);
    if (allocPageSection.start <= 0 || allocPageSection.count <= 0) {
        return;
    }
    TMQAddress allocAddress = ADDRESS(allocPageSection.start, 0);
    TMQSize allocCapacity = allocPageSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<MetaPage> lazyPageList(pageSpace, allocAddress, allocCapacity);
    MetaPage metaPage(page, 0, false);
    // Find its position with quick sort algorithm.
    int pos = lazyPageList.FindPosition(metaPage, PageCompare);
    if (pos < 0 || pos >= lazyPageList.GetSize()) {
        return;
    }
    // Get the meta page at pos.
    MetaPage allocPage = lazyPageList.Get(pos);
    if (allocPage.start != metaPage.start) {
        return;
    }
    allocPage.occupied = false;
    lazyPageList.Set(pos, allocPage);
    // Loop to free pages faraway
    MetaPage allocFarawayPage = lazyPageList.Get(lazyPageList.GetSize() - 1);
    while (!allocFarawayPage.occupied) {
        LOG_DEBUG("Deallocate page:%d, %d", allocFarawayPage.start, allocFarawayPage.count);
        pageSpace->Deallocate(allocFarawayPage.start);
        lazyPageList.Remove(lazyPageList.GetSize() - 1);
        allocFarawayPage = lazyPageList.Get(lazyPageList.GetSize() - 1);
    }
}

/*
 * Find a section with its name. If it is not exist, it will be created when the create is true.
 */
/*

根据名称查找一个区段。如果不存在，当create为true时将创建它。 */
MetaSection Persistence::FindSection(const char *sec, bool create) {
    MetaSection section(sec);
    if (!sec) {
        return section;
    }
    // Search for the meta section.
    int secIndex = -1;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        MetaSection indexSection = lazySectionList->Get(i);
        if (strcmp(indexSection.name, sec) == 0) {
            secIndex = i;
            break;
        }
    }
    // The meta section is not exist, but the create is true, so add it to the lazySectionList first.
    if (secIndex < 0 && create) {
        lazySectionList->Add(section);
        secIndex = lazySectionList->GetSize() - 1;
    }
    if (secIndex >= 0) {
        section = lazySectionList->Get(secIndex);
        // If the pages are not allocated yet, invoke AllocPages to allocate pages.
        if (section.start < 0) {
            section.start = AllocPages(1, &section.count);
            pageSpace->Zero(section.start, 0, section.count * TMQ_PAGE_SIZE);
            lazySectionList->Set(secIndex, section);
        }
    }
    return section;
}

/*
 * Resize the section for reserve count.
 */
/*

为预留计数调整区段的大小。 */
bool Persistence::ResizeSection(MetaSection &section, TMQSize reserve) {
    // If it is not overflow for the reserve count, return true.
    if (!Overflow(section, reserve)) {
        return true;
    }
    MetaPage tmpPage(section.start, section.count);
    int newLen = (int) (section.count + section.count * ALLOC_FACTOR + 1);
    // Allocate new pages for this section.
    int newPage = AllocPages(newLen, &newLen);
    if (newPage < 0) {
        return false;
    }
    // Copy data from old pages to the new pages.
    if (!pageSpace->Copy(newPage, 0, tmpPage.start, 0, tmpPage.count * TMQ_PAGE_SIZE)) {
        return false;
    }
    section.start = newPage;
    section.count = newLen;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        if (strcmp(section.name, lazySectionList->Get(i).name) == 0) {
            lazySectionList->Set(i, section);
            break;
        }
    }
    // Release the old pages.
    DeallocPages(tmpPage.start);
    return true;
}

/*
 * Update section with the parameter section. Two key steps:
 * 1. Find the section. Add it to the lazySectionList if it is not exist.
 * 2. Release old pages and update the meta section in lazySectionList
 */
/*

使用参数区段更新区段。两个关键步骤：
查找区段。如果不存在，则将其添加到lazySectionList中。
释放旧页面并在lazySectionList中更新元区段 */
bool Persistence::UpdateSection(MetaSection &section) {
    // Find the meta section.
    int secIndex = -1;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        MetaSection indexSection = lazySectionList->Get(i);
        if (strcmp(indexSection.name, section.name) == 0) {
            secIndex = i;
            break;
        }
    }
    // Add it to lazySectionList if it is not exist.
    if (secIndex < 0) {
        lazySectionList->Add(section);
    } else {
        // Update meta section if it is exist.
        MetaSection oldSection = lazySectionList->Get(secIndex);
        DeallocPages(oldSection.start);
        lazySectionList->Set(secIndex, section);
    }
    return true;
}

/*
 * Erase section, steps as fellow:
 * 1. Find the meta section named sec.
 * 2. Release all pages that owned by all SecAlloc(lazySectionList)
 * 3. Clear all of the elements in the lazySectionList
 */
/*

擦除区段，步骤如下：
查找名为sec的元区段。
释放所有由所有SecAlloc（lazySectionList）拥有的页面
清除lazySectionList中的所有元素 */
void Persistence::EraseSection(const char *sec) {
    // Find the meta section named sec.
    int secIndex = -1;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        MetaSection indexSection = lazySectionList->Get(i);
        if (strcmp(indexSection.name, sec) == 0) {
            secIndex = i;
            break;
        }
    }
    if (secIndex < 0) {
        return;
    }
    // Loop for calculating the pages, and Release all pages that owned by all
    // SecAlloc(lazySectionList)
    MetaSection metaSection = lazySectionList->Get(secIndex);
    TMQAddress address = ADDRESS(metaSection.start, 0);
    TMQSize capacity = metaSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> lazyAllocList(pageSpace, address, capacity);
    SecAlloc metaAlloc;
    int page = -1;
    for (int i = 0; i < lazyAllocList.GetSize(); ++i) {
        metaAlloc = lazyAllocList.Get(i);
        if (PAGE(metaAlloc.address) == page) {
            continue;
        }
        page = PAGE(metaAlloc.address);
        DeallocPages(page);
    }
    //Clear all of the SecAlloc.
    lazyAllocList.Clear();
}

/*
 * Move section to new page specified by parameter pages.
 */
/*

将区段移动到由参数pages指定的新页面。 */
bool Persistence::MoveSection(MetaSection &section, int page, TMQSize size) {
    // Check parameters
    if (page < 0 || size == 0 || size == -1) {
        return false;
    }
    // Copy data from old pages to the new pages.
    if (section.start >= 0 && section.count > 0) {
        bool suc = pageSpace->Copy(page, 0, section.start, 0,
                                   section.count * TMQ_PAGE_SIZE);
        if (!suc) {
            return false;
        }
    }
    // After copying success, update the meta section in lazySectionList
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        MetaSection indexSection = lazySectionList->Get(i);
        if (strcmp(section.name, indexSection.name) == 0) {
            indexSection.start = page;
            indexSection.count = (int) size;
            lazySectionList->Set(i, indexSection);
            break;
        }
    }
    return true;
}

/*
 * Check whether the section is overflow with the expand count reserved.
 */
/*

检查在预留扩展计数的情况下，区段是否溢出。 */
bool Persistence::Overflow(const MetaSection &section, TMQSize expand) {
    // Check parameters.
    if (section.start < 0 || section.count <= 0 || expand == 0) {
        return false;
    }
    // If the section is SECTION_ALLOC, we use LazyLinearList<MetaPage> to check the overflow,
    // if the section is others, we use LazyLinearList<SecAlloc> to check the overflow.
    TMQAddress address = ADDRESS(section.start, 0);
    TMQSize capacity = section.count * TMQ_PAGE_SIZE;
    if (strcmp(section.name, SECTION_ALLOC) == 0) {
        LazyLinearList<MetaPage> lazyPageList(pageSpace, address, capacity);
        return lazyPageList.GetSize() + expand > lazyPageList.GetCapacity();
    } else {
        LazyLinearList<SecAlloc> lazyAllocList(pageSpace, address, capacity);
        return lazyAllocList.GetSize() + expand > lazyAllocList.GetCapacity();
    }
}

/*
 * Get the allocated page size associated with specified page.
 */
/*
获取与指定页面关联的分配页面大小。
*/
int Persistence::GetAllocPageSize(int page) {
    // Find the meta section first.
    MetaSection pageSection;
    for (int i = 0; i < lazySectionList->GetSize(); ++i) {
        MetaSection indexSection = lazySectionList->Get(i);
        if (strcmp(lazySectionList->Get(i).name, SECTION_ALLOC) == 0) {
            pageSection = indexSection;
            break;
        }
    }
    if (pageSection.start >= 0) {
        // Read the meta page information from SECTION_ALLOC and return its count.
        TMQAddress pageAddress = ADDRESS(pageSection.start, 0);
        TMQSize capacity = pageSection.count * TMQ_PAGE_SIZE;
        LazyLinearList<MetaPage> lazyPageList(pageSpace, pageAddress, capacity);
        MetaPage metaPage(page, 0);
        int pos = lazyPageList.FindPosition(metaPage, PageCompare);
        if (pos >= 0 && pos < lazyPageList.GetSize()) {
            MetaPage foundPage = lazyPageList.Get(pos);
            // The meta page is found, return its count.
            if (foundPage.start == page) {
                return (int) foundPage.count;
            }
        }
    }
    return -1;
}

/*
 * Copy data to the end of the destination section space from the source section space. Main steps:
 * 1. Find the meta section for the source and destination linear space.
 * 2. Resize the destination section space for adding new elements.
 * 3. Copy element from the source section space to destination section space one by one.
 */
/*
将数据从源节空间复制到目标节空间的末尾。主要步骤：
查找源和目标线性空间的元节。
调整目标节空间的大小以添加新元素。
逐一将元素从源节空间复制到目标节空间。
*/
bool Persistence::AppendLinearSpace(const char *dst, const char *src) {
    // Find source meta section.
    MetaSection srcSection = FindSection(src, false);
    // Find destination meta section.
    MetaSection dstSection = FindSection(dst, false);
    // Check whether the source meta section and the destination meta section are both valid.
    if (srcSection.start <= 0 || dstSection.start <= 0) {
        return false;
    }
    TMQAddress pageAddress = ADDRESS(dstSection.start, 0);
    TMQSize capacity = dstSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> dstAllocList(pageSpace, pageAddress, capacity);
    pageAddress = ADDRESS(srcSection.start, 0);
    capacity = srcSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> srcAllocList(pageSpace, pageAddress, capacity);
    // Resize the destination space for reserving srcAllocList.GetSize() space.
    if (!ResizeSection(dstSection, srcAllocList.GetSize())) {
        return false;
    }
    // Copy elements one by one to the end of the destination section space.
    for (int i = 0; i < srcAllocList.GetSize(); ++i) {
        SecAlloc srcAlloc = srcAllocList.Get(i);
        dstAllocList.Add(srcAlloc);
    }
    // Clear source section space.
    srcAllocList.Clear();
    return true;
}



