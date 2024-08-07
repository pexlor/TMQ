//
//  Metas.h
//  Metas
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_METAS_H
#define TMQ_METAS_H

#include "Defines.h"
#include "string.h"

/// Const definitions
// The length of a section name. Since we use only one page to save all the section names, the
// storage for section names is scarce, we have to limit the max length of the section name. If
// the TMQ_PAGE_SIZE is 4096, the max count of the section names will be 4096 / 16 = 256.
#define SECTION_NAME_LEN        16
// Page size in byte for tmq page. It should be a multiple of the page_size defined by system.
#define TMQ_PAGE_SIZE           4096
// Get the page index from the tmq address
#define PAGE(address)           (unsigned int)(address >> 32)
// Get the offset from the tmq address
#define OFFSET(address)         (unsigned int)(address)
// Make a tmq address with page and its offset.
#define ADDRESS(page, offset)    ((TMQAddress)page << 32 | (TMQAddress)offset)
// Definition for invalid page.
#define PAGE_NULL               -1
// Definition for invalid offset.
#define OFFSET_NULL             -1
// Definition for invalid tmq address.
#define ADDRESS_NULL            ADDRESS(PAGE_NULL, OFFSET_NULL)
// TMQAddress state
#define ADDRESS_DEFAULT         0
// ADDRESS_ALLOC indicates that this address is in use.
#define ADDRESS_ALLOC           1
// ADDRESS_FREE indicates that this address is free, which can be reused during next allocation.
#define ADDRESS_FREE            2
// ADDRESS_RELEASE indicates that this address is released, which is waiting for remove.
#define ADDRESS_RELEASE         3
/// Type definitions
// TMQAddress 8 byte count: [4: pages][4:offset]
typedef unsigned long long TMQAddress;
// State of this address, allocated, freed or released. see TMQAddress state above.
typedef unsigned char TMQLState;
// A tmq size for the linear space. Because the file on 64bit machine may big, so TMQLSize is
// defined with unsigned long.
typedef unsigned long TMQLSize;

/*
 * Section class. A section represents a continuous linear space, that starts at the page index
 * 'start', and have 'count' page(s).
 */
class MetaSection {
public:
    // Name of the section.
    char name[SECTION_NAME_LEN];
    // Start index of this section
    int start;
    // Page count of this section.
    int count;

    /**
     * Default constructor.
     */
    MetaSection() : name{0}, start(-1), count(0) {

    }

    /**
     * Construct a MetaSection with name
     * @param name, a const pointer to the name, the max length of the name is limited to
     *  SECTION_NAME_LEN
     */
    explicit MetaSection(const char *name) : name{0}, start(-1), count(0) {
        strncpy(this->name, name, sizeof(this->name));
    }

    /**
     * Construct a MetaSection with name, start index, page count.
     * @param name, a pointer to the name, whose length limits to SECTION_NAME_LEN.
     * @param s, the start page.
     * @param c, continuous page count.
     */
    MetaSection(const char *name, int s, int c) : name{0}, start(s), count(c) {
        strncpy(this->name, name, sizeof(this->name));
    }

    /**
     * Override the operator '='
     * @param section, the original section
     * @return, a reference to *this.
     */
    MetaSection &operator=(const MetaSection &section) {
        this->count = section.count;
        this->start = section.start;
        strncpy(this->name, section.name, sizeof(this->name));
        return *this;
    }
};

/**
 * MetaPage describes a continuous page space. It describes the occupied state at the same time.
 */
class MetaPage {
public:
    // start index of the pages.
    unsigned int start;
    // the count of the pages.
    unsigned int count;
    // a boolean value indicate whether it is occupied or not. It can be reallocated if the value
    // of the occupied is false.
    bool occupied;

    /**
     * Default constructor for the MetaPage.
     */
    MetaPage() : start(0), count(0), occupied(false) {};

    /**
     * Construct MetaPage with start, count and occupied state.
     * @param start, start index of the pages.
     * @param count, count of the pages.
     * @param occupied, a boolean value indicate whether the MetaPage is occupied or not.
     */
    MetaPage(unsigned int start, unsigned int count, bool occupied = false)
            : start(start), count(count), occupied(occupied) {}
};

/**
 * MetaAlloc is used to describe a continuous linear space to allocate or allocated. It has the
 * members address and its size in bytes.
 */
class MetaAlloc {
public:
    // A tmq address.
    TMQAddress address;
    // The size of the MetaAlloc in bytes.
    TMQSize size;

    /**
     * Construct a MetaAddress with tmq address and its size.
     * @param address, address to the allocation.
     * @param size, size of the allocation in bytes.
     */
    MetaAlloc(TMQAddress address, TMQSize size) : address(address), size(size) {}

    /**
     * Default constructor.
     */
    MetaAlloc() : address(ADDRESS_NULL), size(0) {}
};

/**
 * Allocation information by section space. It is based on MetaAlloc, and extends a secAddress and
 * a state of this allocation. The secAddress is used to record the address in section space, and
 * and the state is used to indicate whether the allocation is in use or free. The state is useful
 * for address reuse.
 */
class SecAlloc : public MetaAlloc {
public:
    // a tmq address in section space.
    TMQAddress secAddress;
    // the state of this allocation, ADDRESS_ALLOC, ADDRESS_FREE, ADDRESS_RELEASE.
    TMQLState state;

    SecAlloc() : MetaAlloc(), state(ADDRESS_DEFAULT), secAddress(ADDRESS_NULL) {}

    SecAlloc(TMQAddress sec, TMQAddress tmq, TMQSize size, TMQLState state)
            : MetaAlloc(tmq, size), state(state), secAddress(sec) {}
};


#endif //TMQ_METAS_H
