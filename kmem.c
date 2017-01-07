/*
 * mem.c
 *
 *  Created on: Nov 17, 2016
 *      Author: ogruber
 */

#include "board.h"
#include "kmem.h"

#define VERBOSE_CLEANUP

struct __attribute__ ((__packed__,aligned(4))) _chunk {
  uint16_t size;
  struct _chunk *next;
};

ALWAYS_INLINE
void* chunk_data(struct _chunk *chunk) {
  return ((void*)chunk) + sizeof(struct _chunk);
}

ALWAYS_INLINE
struct _chunk* chunk_of(void *addr) {
  return (addr - sizeof(struct _chunk));
}


struct __attribute__ ((__packed__,aligned(4))) space_page {
  uint16_t start;
  uint16_t end;
  uint16_t nchunks;
  uint16_t offset;
  uint16_t free;
  struct space_page *next;
  struct space_valloc *allocator;
};

ALWAYS_INLINE
struct space_page* space_page_of(void* addr) {
  addr = HAL_PAGE_OF(addr) + HAL_PAGE_SIZE - sizeof(struct space_page);
  return (struct space_page*)addr;
}

struct __attribute__ ((__packed__,aligned(4))) space_valloc {
  uintptr_t low,high;
  struct space_page *first;
  struct space_page *pages;
  uint32_t npages;
  uint32_t nzpages;
  struct _chunk *holes;
  uint32_t nholes;
  struct {
    struct space_page *pages;
    uint16_t npages;
  } free;
  uint32_t nchunks;
#ifdef CONFIG_SPACE_STATS
  uint64_t allocated;
#endif
};

extern uint32_t _kheap_low;
extern uint32_t _kheap_high;

/**
 * Initializes a new page, adding it to the list of free empty pages.
 *
 * Note how the struct space_page is allocated within the page itself,
 * towards the end of the page.
 *
 */
static
struct space_page *
space_page_init(struct space_valloc* alloc, uintptr_t addr, uint32_t reserved) {

  // compute the offset in the page where to place the struct space_page
  int page_offset = HAL_PAGE_SIZE - sizeof(struct space_page);

  // finally allocate the stuct space_page in the page it represents
  struct space_page* page;
  page = (struct space_page*) (addr + page_offset);

  page->allocator = alloc;
  page->start = reserved;
  page->end = page_offset;
  page->free = reserved;
  page->nchunks = 0;
  page->offset = reserved;
  page->next = NULL;

  return page;
}

struct space_valloc _alloc;

/*
 * Initialization of the malloc/free subsystem,
 * which is a simple memory allocator of variable-size chunks.
 *
 * The allocator is given an initial region of memory to work with,
 * see the _heap_low and _heap_high symbols in the linker script.
 *
 * The memory region is divided in individual pages, from which chunks
 * are allocated. Therefore, chunks must be smaller than a page.
 *
 * When freed, chunks are added to a hole list that is scanned when
 * trying to malloc some memory. The intent is to reuse freed memory.
 * However, scanning this list becomes rapidly a bottleneck, which can be
 * reduced if we can cleanup the space. The idea is to use holes only
 * for pages partially allocated.
 */
void space_valloc_init() {

  struct space_valloc* alloc = &_alloc;

  alloc->free.pages = NULL;
  alloc->free.npages = 0;
  alloc->holes = NULL;
  alloc->nholes = 0;

  alloc->first = NULL;
  alloc->pages = NULL;
  alloc->npages = 0;
  alloc->nzpages = 0;

  alloc->low = (uintptr_t)&_kheap_low;
  alloc->high = (uintptr_t)&_kheap_high;

#ifdef CONFIG_SPACE_STATS
  alloc->allocated = 0;
#endif

  uintptr_t addr = (uintptr_t)&_kheap_low;
  uintptr_t next = addr + HAL_PAGE_SIZE;

  struct space_page *page;

  page = space_page_init(alloc,addr,0);

  alloc->first = page;
  alloc->pages = page;
  alloc->npages = 1;
  alloc->nzpages = 1;

  addr += HAL_PAGE_SIZE;
  next = addr + HAL_PAGE_SIZE;

  while (next <= (uintptr_t)&_kheap_high) {
    page = space_page_init(alloc,addr,0);
    page->next = alloc->free.pages;
    alloc->free.pages = page;
    alloc->free.npages++;

    addr += HAL_PAGE_SIZE;
    next = addr + HAL_PAGE_SIZE;
  }
  kprintf("Initialized malloc/free, region is [0x%x 0x%x[ size=%d \n ",
      alloc->low,alloc->high, (alloc->high-alloc->low));
  kprintf("    -> %d allocated pages \n",alloc->npages);
  kprintf("    -> %d empty pages \n",alloc->free.npages);
}

/**
 * Allocate a chunk of memory.
 * Note there is a maximum chunk size.
 */
void* kmalloc(uint32_t size) {

  struct space_valloc* alloc = &_alloc;

  size = ALIGN32(size);
  if (size > MAX_HOLE_SIZE)
    panic(666, "Size too large");

  int length = size + sizeof(struct _chunk);

  struct space_page* page = NULL;
  struct _chunk *chunk;
  struct _chunk *prev = NULL;
  struct _chunk *hole;
  hole = alloc->holes;
  while (hole) {
    if (hole->size >= length) {
      uint32_t remaining = hole->size - length;
      if (remaining > MIN_HOLE_SIZE) { // do we split the hole
        hole->size = remaining;
        chunk = ((void*) hole) + remaining;
        chunk->next = NULL;
        chunk->size = size;
        page = space_page_of(chunk);
        goto done;
      }
      if (prev)
        prev->next = hole->next;
      else
        alloc->holes = hole->next;
      hole->next = NULL;
      alloc->nholes--;
      chunk = (void*) hole;
      /*
       * do not update the size, we must keep the true size
       * not the allocated size. indeed, upon the next free,
       * we would have lost the knowledge of the real size of the hole.
       */
      page = space_page_of(chunk);
      goto done;
    } else
      prev = hole;
    hole = hole->next;
  }
  page = alloc->pages;
  uint32_t offset = page->offset + length;
  if (offset > page->end) {
    if (alloc->free.pages == NULL && alloc->nzpages != 0)
      space_valloc_cleanup();
    if (alloc->free.pages) {
      page = alloc->free.pages;
      alloc->free.pages = page->next;
      alloc->free.npages--;
      page->nchunks = 0;
      page->offset = page->start;
      page->free = page->start;
      page->next = NULL;
    } else {
      panic(-1,"PANIC: OUT OF MEMORY \n\r");
    }
    page->next = alloc->pages;
    alloc->pages = page;
    alloc->npages++;
    alloc->nzpages++;
  }
  chunk = (HAL_PAGE_OF(page) + page->offset);
  chunk->size = size;
  page->offset += length;

  done: /* DONE */
  if (page->nchunks == 0) {
    alloc->nzpages--;
    assert(alloc->nzpages <= alloc->npages, "FIXME");
  }
  page->nchunks++;
  alloc->nchunks++;
#ifdef CONFIG_SPACE_STATS
  /*
   * use the chunk->size here, not size,
   * the hole size might be larger.
   */
  alloc->allocated += chunk->size;
#endif

  void *addr = chunk_data(chunk);
  return addr;
}

/**
 * Free an allocated chunk of memory.
 */
void kfree(void* addr) {
  struct _chunk* hole = chunk_of(addr);
  struct space_page* page = space_page_of(addr);
  struct space_valloc* alloc = page->allocator;

#ifdef CONFIG_SPACE_STATS
  assert(alloc->allocated >= hole->size,
      "Botched valloc: allocated=%llu size=%llu", alloc->allocated, hole->size);
  alloc->allocated -= hole->size;
#endif

  hole->next = alloc->holes;
  alloc->holes = hole;
  alloc->nholes++;
  alloc->nchunks--;

  page->nchunks--;
  if (page->nchunks == 0) {
    alloc->nzpages++;
    assert(alloc->nzpages <= alloc->npages, "FIXME");
  }
}

/**
 * Attempts to gain back free pages, that is, to remove from the hole list
 * all the holes that belong to pages with only holes. If we do that,
 * some pages will become empty, that is, pages with no allocated chunks
 * and no freed holes. Such pages could therefore be returned to the list
 * of free pages.
 *
 * This cleanup optimizes the malloc performance since searching through the
 * hole list is expensive as it grows. In contrast, allocating from free pages
 * is fast. So cleanup is efficient, if it manages to return pages to the list
 * of free pages. Otherwise, it is just pure overhead.
 *
 * Note that we never free the first page.
 */
void space_valloc_cleanup() {

  struct space_valloc* alloc=&_alloc;

  if (alloc->npages == 1)
    return;
  struct _chunk *prev = NULL;
  struct _chunk *hole;
  uint32_t nholes = 0;
  uint32_t npages = 0;
  hole = alloc->holes;
  while (hole) {
    struct space_page* page;
    struct _chunk *next = hole->next;
    page = space_page_of(hole);
    if (page->nchunks == 0 && page != alloc->first) {
      if (prev)
        prev->next = next;
      else
        alloc->holes = next;
      alloc->nholes--;
      nholes++;
    } else
      prev = hole;
    hole = next;
  }
  {
    struct space_page* prev = NULL;
    struct space_page* page;
    page = alloc->pages;
    while (page) {
      struct space_page* next;
      next = page->next;
      if (page->nchunks == 0 && page != alloc->first) {
        if (prev)
          prev->next = next;
        else
          alloc->pages = next;
        alloc->npages--;
        alloc->nzpages--;
        assert(alloc->nzpages <= alloc->npages, "FIXME");
        npages++;
        page->next = alloc->free.pages;
        alloc->free.pages = page;
        alloc->free.npages++;
      } else
        prev = page;
      page = next;
    }
  }
#ifdef VERBOSE_CLEANUP
  if (npages) {
    kprintf("# valloc: freed npages=%d from nholes=%d \n",
        npages,nholes);
#ifdef CONFIG_SPACE_STATS
    kprintf("    -> npages=%d allocated=%d \n",alloc->npages,alloc->allocated);
#else
    kprintf("    -> npages=%d \n",alloc->npages);
#endif
    kprintf("    -> %d empty pages \n",alloc->free.npages);
  }
#endif
}
