#include "SectionSpace.h"

/*
 * 构造函数，这将分配名称，并查找与此部分空间关联的 MetaSection。
 */
SectionSpace::SectionSpace(Persistence *persist, const char *sec)
 : SectionAllocator(), name{0}, persist(persist), releaseCount(0) {
    strncpy(this->name, sec, sizeof(this->name));
    // 查找或创建元部分。
    metaSection = persist->FindSection(name, true);
}

/*
 * 析构函数。
 */
SectionSpace::~SectionSpace() {
    this->persist = nullptr;
}

/*
 * 在每次调用 Allocate() 期间获取 alloc id，并检查它是否达到最大值。
 * 如果 allocId 达到 ALLOC_ID_MAX，我们将切换 ResetSpace 任务。然后在其父级中调用 Allocate()。
 */
TMQAddress SectionSpace::Allocate(TMQLSize length) {
    // 查找最后一个 allocId 键
    LazyLinearList<SecAlloc> *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (lazyLinearList && lazyLinearList->GetSize() > 0) {
        SecAlloc lastAlloc = lazyLinearList->Get((long) lazyLinearList->GetSize() - 1);
        allocId = SECTION_ALLOC_ID(lastAlloc.secAddress);
    }
    // 如果 allocId 达到 ALLOC_ID_MAX，ResetSpace()
    if (allocId == ALLOC_ID_MAX) {
        allocId = ResetSpace();
    }
    return SectionAllocator::Allocate(length);
}

/**
 * 释放部分地址。实际的 Deallocate 将在其父级发生，此方法的另一个功能是计算释放比率并切换 ResetSpace 任务。
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
    if ((float)releaseCount / (float)tmqSize > SPACE_RESET_THRESHOLD) {
        LOG_DEBUG("Deallocate ResetSpace: %d %d", releaseCount, tmqSize);
        ResetSpace();
    }
}

/*
 * 重置并缩小部分空间。
 */
unsigned int SectionSpace::ResetSpace() {
    // 获取用于读取的懒惰线性列表。
    auto *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (lazyLinearList == nullptr) {
        return 0;
    }
    // 申请一些新页面
    int size = metaSection.count;
    int page = persist->AllocPages(size, &size);
    // 分配失败。
    LOG_DEBUG("SectionAllocator::ResetSpace %d %d", page, size);
    if (page < 0) {
        return 0;
    }
    // 初始化新页面空间。
    persist->GetPageSpace()->Zero(page, 0, size * TMQ_PAGE_SIZE);
    TMQAddress address = ADDRESS(page, 0);
    TMQSize capacity = size * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> newLinearList(persist->GetPageSpace(), address, capacity);
    // 循环复制未释放的 SecAlloc。
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
    // 更新到新部分。
    persist->UpdateSection(metaSection);
    return newLinearList.GetSize();
}

/*
 * 从部分空间读取数据。首先，查找与地址关联的 SecAlloc，然后计算实际数据的 tmq 地址。
 */
TMQLSize SectionSpace::Read(TMQAddress address, void *buf, TMQLSize length) {
    if (!persist) {
        return -1;
    }
    // 查找与地址关联的 SecAlloc。
    SecAlloc metaAlloc;
    if (FindAlloc(address, metaAlloc) < 0) {
        return -1;
    }
    // 将 tmq 地址转换为页面及其偏移量。
    auto page = PAGE(metaAlloc.address);
    auto offset = OFFSET(metaAlloc.address);
    return persist->GetPageSpace()->Read(page, offset, buf, length);
}

/*
 * 将数据写入部分空间。
 */
TMQLSize SectionSpace::Write(TMQAddress address, void *data, TMQLSize length) {
    if (!persist) {
        return -1;
    }
    // 查找与地址关联的 SecAlloc。
    SecAlloc metaAlloc;
    if (FindAlloc(address, metaAlloc) < 0) {
        return -1;
    }
    // 将 tmq 地址转换为页面及其偏移量。
    auto page = PAGE(metaAlloc.address);
    auto offset = OFFSET(metaAlloc.address);
    return persist->GetPageSpace()->Write(page, offset, data, length);
}

/*
 * 从源部分地址复制数据到目标部分地址。
 */
void SectionSpace::Copy(TMQAddress dst, TMQAddress src, TMQLSize length) {
    if (!persist) {
        return;
    }
    // 首先查找源和目标 SecAlloc。
    SecAlloc dstAlloc, srcAlloc;
    if (FindAlloc(dst, dstAlloc) < 0 || FindAlloc(src, srcAlloc) < 0) {
        return;
    }
    // 将 tmq 地址转换为页面及其偏移量。
    auto sp = PAGE(srcAlloc.address);
    auto sf = OFFSET(srcAlloc.address);
    auto dp = PAGE(dstAlloc.address);
    auto df = OFFSET(dstAlloc.address);
    persist->GetPageSpace()->Copy(dp, df, sp, sf, length);
}

/*
 * 将内容设置为零。
 */
void SectionSpace::Zero(TMQAddress address, TMQLSize length) {
    if (!persist) {
        return;
    }
    // 查找与地址关联的 SecAlloc。
    SecAlloc metaAlloc;
    if (FindAlloc(address, metaAlloc) < 0) {
        return;
    }
    // 将 tmq 地址转换为页面及其偏移量。
    auto page = PAGE(metaAlloc.address);
    auto offset = OFFSET(metaAlloc.address);
    persist->GetPageSpace()->Zero(page, offset, length);
}

/*
 * 获取此部分的名称。
 */
const char *SectionSpace::GetName() {
    return name;
}

/*
 * 获取懒惰线性列表的指针。三个关键步骤。
 * 1. 查找并创建与此部分空间关联的元部分。
 * 2. 调整部分大小以满足所需的保留计数。
 * 3. 将本地懒惰线性列表分配给 lazyAllocList 并返回它。
 */
LazyLinearList<SecAlloc> *SectionSpace::GetLazyAllocList(TMQSize reserve) {
    if (persist == nullptr) {
        return nullptr;
    }
    // 如果无效，查找并创建与此部分空间关联的元部分。
    if (metaSection.start <= 0) {
        metaSection = persist->FindSection(name, true);
    }
    // 调整部分大小以满足所需的保留计数。
    if (!persist->ResizeSection(metaSection, reserve)) {
        return nullptr;
    }
    TMQAddress pageAddress = ADDRESS(metaSection.start, 0);
    TMQSize capacity = metaSection.count * TMQ_PAGE_SIZE;
    LazyLinearList<SecAlloc> lazyLinearList(persist->GetPageSpace(), pageAddress, capacity);
    // 将本地懒惰线性列表分配给 lazyAllocList 并返回它。
    lazyAllocList = lazyLinearList;
    return &lazyAllocList;
}

/*
 * 释放部分分配。
 */
bool SectionSpace::ReleaseAlloc(TMQAddress secAddress) {
    // 查找 SecAlloc，如果不存在，忽略此调用。
    SecAlloc secAlloc;
    int position = FindAlloc(secAddress, secAlloc);
    if (position < 0) {
        return false;
    }
    // 将其状态更改为 ADDRESS_RELEASE。
    secAlloc.state = ADDRESS_RELEASE;
    auto *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (position >= lazyLinearList->GetSize()) {
        return false;
    }
    lazyLinearList->Set(position, secAlloc);
    // 计算释放比率以切换 ResetSpace 任务。
    releaseCount++;
    if ((float)releaseCount / (float)lazyLinearList->GetSize() > SPACE_RESET_THRESHOLD) {
        ResetSpace();
    }
    return true;
}

/*
 * 附加具有指定状态的 SecAlloc。
 */
TMQAddress SectionSpace::AppendAlloc(TMQAddress tmqAddress, TMQSize size, TMQLState state) {
    // 获取用于写入的懒惰线性列表。
    auto *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_WRITE);
    if (lazyLinearList == nullptr) {
        return false;
    }
    TMQAddress secAddress = SECTION_ADDRESS(lazyLinearList->GetSize(), ++allocId);
    SecAlloc secAlloc(secAddress, tmqAddress, size, state);
    // 保存 secAlloc 并返回部分地址。
    if (lazyLinearList->Add(secAlloc) >= 0) {
        return secAddress;
    }
    return ADDRESS_NULL;
}

/*
 * 获取指定页面的已分配大小。直接在 persistence 中调用 GetAllocPageSize。
 */
int SectionSpace::GetAllocPageSize(int page) {
    return persist->GetAllocPageSize(page);
}

/*
 * 释放指定页面。
 */
void SectionSpace::DeallocPages(int page) {
    persist->DeallocPages(page);
}

/*
 * 分配页面，直接委托此调用到 persistence 中的 AllocPages。
 */
int SectionSpace::AllocPages(int size, int *allocSize) {
    return persist->AllocPages(size, allocSize);
}

/*
 * 读取此部分空间中的 MetaAlloc 列表。
 */
bool SectionSpace::GetAllocList(List<MetaAlloc> &allocList) {
    // 获取用于读取的懒惰线性列表。
    LazyLinearList<SecAlloc> *lazyLinearList = GetLazyAllocList(LAZY_RESERVE_READ);
    if (lazyLinearList == nullptr) {
        return false;
    }
    // 循环懒惰线性列表并将 ADDRESS_ALLOC 状态的 MetaAlloc 保存到 allocList。
    for(int i = 0; i < lazyLinearList->GetSize(); ++i) {
        SecAlloc indexAlloc = lazyLinearList->Get(i);
        if (indexAlloc.state == ADDRESS_ALLOC) {
            MetaAlloc metaAlloc(indexAlloc.secAddress, indexAlloc.size);
            allocList.Add(metaAlloc);
        }
    }
    return true;
}