//
//  Persistence.h
//  Persistence
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "Metas.h"
#include "LinearSpace.h"
#include "PageSpace.h"
#include "LazyLinearList.h"
#include "List.h"

/**
 * A interface definition for the data persistence. It provides functions to manage the section
 * space, such as create, drop and find method. The other functions, EraseLinearSpace,
 * AppendLinearSpace will also the APIs to manage the section space. With the section space, One can
 * allocate, deallocate the linear space as need, and read/write data from/to the persistence.
 */
class IPersistence {
public:
    /**
     * Create a named linear space.
     * @param name, a pointer to the name.
     * @return, a pointer to the created section space.
     */
    virtual ISectionSpace *CreateLinearSpace(const char *name) = 0;

    /**
     * Drop a linear space with specified.
     * @param name, a pointer to the name.
     */
    virtual void DropLinearSpace(const char *name) = 0;

    /**
     * Find a linear space with the name.
     * @param name, a pointer to the name.
     * @return a pointer to the section space, nullptr will be returned if it is not exist.
     */
    virtual ISectionSpace *FindLinearSpace(const char *name) = 0;

    /**
     * Erase a linear space, clear all data in that section space.
     * @param name, a pointer to the name.
     */
    virtual void EraseLinearSpace(const char *name) = 0;

    /**
     * Copy data to the end of the destination section space from the source section space.
     * @param dst, the destination section name.
     * @param src, the source section name.
     * @return, bool, a boolean value indicate whether it is success or not.
     */
    virtual bool AppendLinearSpace(const char *dst, const char *src) = 0;
};

/**
 * The implementation for the IPersistence. This class will be used to manage the page space and
 * multiply section spaces, and add or remove meta sections.
 */
class Persistence : public IPersistence {
private:
    // A pointer to the page space, MemSpace or FileSpace.
    IPageSpace *pageSpace;
    // Section space list.
    List<ILinearSpace *> sectionSpaces;
    // Meta section list.
    LazyLinearList<MetaSection> *lazySectionList;

public:
    /**
     * Construct a persistence with a page space.
     * @param pageSpace, a pointer to the page space.
     */
    Persistence(IPageSpace *pageSpace);

    /**
     * Destructor for the Persistence.
     */
    ~Persistence();

    /**
     * Get method to the page space.
     * @return a pointer to the page space.
     */
    IPageSpace *GetPageSpace();

    /**
     * Find a meta section, if it is not exits, the meta section will be created.
     * @param sec, a pointer to the secion name.
     * @param create, a boolean value indicate whether create a new meta section or not.
     * @return a meta section named by sec.
     */
    MetaSection FindSection(const char *sec, bool create = true);

    /**
     * Erase a section, clear all data in the section space named sec.
     * @param sec, a const pointer to the section name.
     */
    void EraseSection(const char *sec);

    /**
     * Check whether the section space is overflow with expand or not.
     * @param section, meta section to check.
     * @param expand, the reserve count to check.
     * @return, bool, a boolean value indicate whether the it is overflow with the reserved count.
     */
    bool Overflow(const MetaSection &section, TMQSize expand);

    /**
     * Resize the meta section with the reserve count, default reserve is 1.
     * @param section, a meta section to reserve.
     * @param reserve, the count to reserve.
     * @return bool, a boolean value indicate whether the resize operation is success or not.
     */
    bool ResizeSection(MetaSection &section, TMQSize reserve = 1);

    /**
     * Update the meta section. This will deallocate page in meta section of persistence and update
     * to page in parameter named section.
     * @param section, the section to update.
     * @return bool, a boolean value indicate whether this update is success or not.
     */
    bool UpdateSection(MetaSection &section);

    /**
     * Move a meta section and its section space in to new page.
     * @param section, meta section to move.
     * @param page, new destination page.
     * @param size, size of the new page.
     * @return, bool, a boolean value indicates whether the move is success or not.
     */
    bool MoveSection(MetaSection &section, int page, TMQSize size);

    /**
     * Method to do page reuse. When pages in the middle of the linear space, deallocating these
     * pages is no effect for the final length of the linear space. So, if a deallocating page is in
     * the middle of the linear space, it will not release immediately, on the contrary, it will be
     * put into the freed page list. The page reuse operation is based on the freed pages and
     * reuse pages ordered from beginning to end.
     * @param size, the size to require.
     * @param real, the real size be allocated.
     * @return page index of this allocation.
     */
    int ReusePages(int size, int *real);

    /**
     * Apply some pages. If there are freed pages, it will do pages reuse firstly. Otherwise it will
     * allocate new pages.
     * @param size, size of the required length.
     * @param real, real length to allocated.
     * @return, page index of this allocation.
     */
    int AllocPages(int size, int *real);

    /**
     * Get the allocated size associated with this page.
     * @param page, page to find.
     * @return allocated size associated with this page.
     */
    int GetAllocPageSize(int page);

    /**
     * Deallocate pages specified by page.
     * @param page, page to deallocate.
     */
    void DeallocPages(int page);

    /**
     * Create a section space with a specified name.
     * @param name, a pointer to the name.
     * @return a pointer to the created section space.
     */
    virtual ISectionSpace *CreateLinearSpace(const char *name);

    /**
     * Drop a linear space with specified name.
     * @param name, a pointer to the name.
     */
    virtual void DropLinearSpace(const char *name);

    /**
     * Find a section with a name.
     * @param name, a pointer to the name.
     * @return a pointer to the section space.
     */
    virtual ISectionSpace *FindLinearSpace(const char *name);

    /**
     * Erase a linear space and clear its section space.
     * @param name, a pointer to its name.
     */
    virtual void EraseLinearSpace(const char *name);

    /**
     * Copy data to the end of the destination section space from the source section space.
     * @param dst, the destination section name.
     * @param src, the source section name.
     * @return, bool, a boolean value indicate whether it is success or not.
     */
    virtual bool AppendLinearSpace(const char *dst, const char *src);
};


#endif //PERSISTENCE_H
