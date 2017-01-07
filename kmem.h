/*
 * mem.h
 *
 *  Created on: Nov 17, 2016
 *      Author: ogruber
 */

#ifndef KMEM_H_
#define KMEM_H_

#define HAL_CACHE_LINE_SIZE 32
#define HAL_PAGE_SIZE 4096
#define HAL_PAGE_MASK (~(HAL_PAGE_SIZE-1))
#define HAL_PAGE_OF(addr) (void*)((uintptr_t)addr & ((uintptr_t)HAL_PAGE_MASK))

/*
 * Nota Bene: the all-casting to (uintptr_t) is mandatory to work on both
 *            32bit and 64bit machines.
 */
#define HAL_CACHE_LINE_GRAIN (HAL_CACHE_LINE_SIZE-1)
#define HAL_ALIGN_CACHE(v) (uintptr_t)((((uintptr_t)(v))+((uintptr_t)HAL_CACHE_LINE_GRAIN))& ~((uintptr_t)HAL_CACHE_LINE_GRAIN))

/*
 * Nota Bene: the all-casting to (uintptr_t) is mandatory to work on both
 *            32bit and 64bit machines.
 */
#define HAL_PAGE_GRAIN (HAL_PAGE_SIZE-1)
#define HAL_ALIGN_PAGE(v) (void*)((((uintptr_t)(v))+((uintptr_t)HAL_PAGE_GRAIN))& ~((uintptr_t)HAL_PAGE_GRAIN))


#define MAX_HOLE_SIZE 3072
#define MIN_HOLE_SIZE 32

void space_valloc_init(void);
void space_valloc_cleanup(void);
void* kmalloc(uint32_t size);
void kfree(void* addr);

#endif /* KMEM_H_ */
