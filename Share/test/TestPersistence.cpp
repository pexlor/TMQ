//
//  TestPersistence.cpp
//  TestPersistence
//
//  Created by  on 2022/9/12.
//  Copyright (c)  Tencent. All rights reserved.
//

#include "TestSuite.h"
#include "Persistence.h"
#include "MemSpace.h"

void TestCreatePersistence() {
    MemSpace memSpace;
    Persistence persist(&memSpace);
    ASSERT_TRUE(persist.GetPageSpace() == &memSpace,
                "The return value by GetPageSpace should be equal to &memSpace.");
}

void TestPersistenceFindSection() {
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    MetaSection metaSection = persist.FindSection(testSection, false);
    ASSERT_TRUE(metaSection.start < 0, "There have not create this section, so find will fail.");
    metaSection = persist.FindSection(testSection, true);
    ASSERT_TRUE(metaSection.start >= 0, "If find section with create, "
                                        "the return value should be success always.");
}

void TestPersistenceSectionOverflow() {
    const int overflowLimit = 1 * TMQ_PAGE_SIZE / sizeof(MetaAlloc) + sizeof(long long);
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    MetaSection metaSection = persist.FindSection(testSection, true);
    bool overflow = persist.Overflow(metaSection, 1);
    ASSERT_TRUE(!overflow, "Call overflow on empty section will return false.");
    overflow = persist.Overflow(metaSection, overflowLimit);
    ASSERT_TRUE(overflow, "Overflow will occur with required size of limit.");
}

void TestPersistenceResizeSection() {
    const int overflowLimit = 1 * TMQ_PAGE_SIZE / sizeof(MetaAlloc) + sizeof(long long);
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    MetaSection metaSection = persist.FindSection(testSection, true);
    bool suc = persist.ResizeSection(metaSection, 1);
    ASSERT_TRUE(suc, "Call ResizeSection on empty section will return ok.");
    MetaSection orgSection = metaSection;
    suc = persist.ResizeSection(metaSection, overflowLimit);
    ASSERT_TRUE(suc, "Call resize section should be success, "
                     "because ResizeSection will reallocate pages");
    ASSERT_TRUE(orgSection.start != metaSection.start,
                "After resize with overflow limit size, "
                "the meta section will be change to new section");
}

void TestPersistenceUpdateSection() {
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    // find and create the test section.
    persist.FindSection(testSection, true);
    // create a new test section, then allocate pages for it.
    MetaSection newSection(testSection);
    int size;
    newSection.start = persist.AllocPages(1, &size);
    newSection.count = size;
    bool suc = persist.UpdateSection(newSection);
    ASSERT_TRUE(suc, "Update an existed section should be success.");
}

void TestPersistenceMoveSection() {
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    MetaSection metaSection = persist.FindSection(testSection, true);
    MetaSection newSection(testSection);
    int size = 1;
    newSection.start = persist.AllocPages(size, &size);
    newSection.count = size;
    bool suc = persist.MoveSection(metaSection, newSection.start, newSection.count);
    ASSERT_TRUE(suc, "Move section should be success with valid page and count");
}

void TestPersistenceAllocatePages() {
    MemSpace memSpace;
    Persistence persist(&memSpace);
    int size = 1;
    int start = persist.AllocPages(size, &size);
    ASSERT_TRUE(start >= 0 && size >= 1, "AllocPages should return valid page index and size.");
}

void TestPersistenceDeallocatePages() {
    MemSpace memSpace;
    Persistence persist(&memSpace);
    int size = 1;
    int start = persist.AllocPages(size, &size);
    ASSERT_TRUE(start >= 0 && size >= 1, "DeallocPages should be success with valid params");
    persist.DeallocPages(start);
}

void TestPersistenceReusePages() {
    MemSpace memSpace;
    Persistence persist(&memSpace);
    int size = 1;
    int start = persist.AllocPages(size, &size);
    int secondSize = 2;
    int second = persist.AllocPages(secondSize, &secondSize);
    persist.DeallocPages(start);
    int reuse = persist.ReusePages(size, &size);
    ASSERT_TRUE(start == reuse && size == 1, "There will be a idle page by deallocate, "
                                             "reuse should return that page and size.");
}

void TestPersistenceGetPageSize() {
    MemSpace memSpace;
    Persistence persist(&memSpace);
    int size = 1;
    int start = persist.AllocPages(size, &size);
    ASSERT_TRUE(persist.GetAllocPageSize(start) == size,
                "The page count should be equal to size by alloc");
}

void TestCreateLinearSpace() {
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    ILinearSpace *space = persist.CreateLinearSpace(testSection);
    ASSERT_TRUE(space, "CreateLinearSpace should be success.");
}

void TestDestroyLinearSpace() {
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    ILinearSpace *space = persist.CreateLinearSpace(testSection);
    ASSERT_TRUE(space, "DropLinearSpace should be success.");
    persist.DropLinearSpace(testSection);
}

void TestFindLinearSpace() {
    const char *testSection = "Test";
    MemSpace memSpace;
    Persistence persist(&memSpace);
    persist.CreateLinearSpace(testSection);
    ILinearSpace *found = persist.FindLinearSpace(testSection);
    ASSERT_TRUE(found, "FindLinearSpace should be success, because the section is exists.");
    persist.DropLinearSpace(testSection);
}

void TestEraseLinearSpace() {
    const char *testSection = "Test";
    const int length = 100;
    MemSpace memSpace;
    Persistence persist(&memSpace);
    persist.CreateLinearSpace(testSection);
    ILinearSpace *found = persist.FindLinearSpace(testSection);
    TMQAddress address = found->Allocate(length);
    ASSERT_TRUE(address != ADDRESS_NULL,
                "EraseLinearSpace should be success, because the section is exists.");
    persist.EraseLinearSpace(testSection);
}

void TestPersistence() {
    TestCreatePersistence();
    TestPersistenceFindSection();
    TestPersistenceSectionOverflow();
    TestPersistenceResizeSection();
    TestPersistenceUpdateSection();
    TestPersistenceMoveSection();
    TestPersistenceAllocatePages();
    TestPersistenceDeallocatePages();
    TestPersistenceReusePages();
    TestPersistenceGetPageSize();
    TestCreateLinearSpace();
    TestDestroyLinearSpace();
    TestFindLinearSpace();
    TestEraseLinearSpace();
}

