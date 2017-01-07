#ifndef MMU_H_
#define MMU_H_

#include "board.h"

#ifdef vexpress_a9
/*
 * armv7-a-r-manual.pdf
 * B3.5.1 Short-descriptor translation table format descriptors (page 1326)
 */

#define FIRST_LEVEL_ENTRY 0x1000
#define FIRST_LEVEL_TABLE_SIZE  0x4000

#define SECOND_LEVEL_ENTRY 0x100
#define SECOND_LEVEL_TABLE_SIZE  0x400

#define SECTION_SIZE 0x100000
struct section{
	unsigned int type :2; //should be 0b10
	unsigned int buffurable :1;
	unsigned int cacheable :1;
	unsigned int execute_never :1;
	unsigned int domain :4;
	unsigned int :1;
	unsigned int ap :2;
	unsigned int tex :3;
	unsigned int ap2 :1;
	unsigned int shareable :1;
	unsigned int not_global :1;
	unsigned int :2;
	unsigned int section :12;
};

struct page_table{
	unsigned int type :2; //should be 0b01
	unsigned int :3;
	unsigned int domain :4;
	unsigned int :1;
	unsigned int section :22;
};

#define LARGE_PAGE_SIZE 0x10000
struct large_page{
	unsigned int type :2; //should be 0b01
	unsigned int buffurable :1;
	unsigned int cacheable :1;
	unsigned int ap :2;
	unsigned int :3;
	unsigned int ap2 :1;
	unsigned int shareable :1;
	unsigned int not_global :1;
	unsigned int tex :3;
	unsigned int execute_never :1;
	unsigned int page :16;
};

#define SMALL_PAGE_SIZE 0x1000
struct small_page{
	unsigned int execute_never :1;
	unsigned int type :1; //should be 0b1
	unsigned int buffurable :1;
	unsigned int cacheable :1;
	unsigned int ap :2;
	unsigned int tex :3;
	unsigned int ap2 :1;
	unsigned int shareable :1;
	unsigned int not_global :1;
	unsigned int page :20;
};

// just a structure with all permission
struct permission{
	unsigned int buffurable;
	unsigned int cacheable;
	unsigned int ap;
	unsigned int ap2;
	unsigned int tex;
	unsigned int shareable;
	unsigned int not_global;
	unsigned int domain;
	unsigned int execute_never;
};

void mmu_init();

uintptr_t create_new_ptp();
void protect_kernel(uintptr_t ptp);

//uintptr_t start_first_mmu();

uintptr_t new_user_ptp();

ALWAYS_INLINE
void set_TTBR0(uint32_t ttbr) {
	__asm__ __volatile("mcr p15, 0, %0, c2, c0, 0" :: "r" (ttbr) : "memory");
}

/* Translation Table Base Control Register
 * N : bits[2..0]
 * 0 to simplify
 */
ALWAYS_INLINE
void set_TTBCR(uint32_t ttbcr) {
	__asm__ __volatile("mcr p15, 0, %0, c2, c0, 2" :: "r" (ttbcr) : "memory");
}

/* Domain Access Control Register
 * 0b00 No access. Any access to the domain generates a Domain fault.
 * 0b01 Client. Accesses are checked against the permission bits in the translation tables.
 * 0b10 Reserved, effect is UNPREDICTABLE .
 * 0b11 Manager. Accesses are not checked against the permission bits in the translation tables.
 */
ALWAYS_INLINE
void set_DACR(uint32_t dacr) {
	__asm__ __volatile("mcr p15, 0, %0, c3, c0, 0" :: "r" (dacr) : "memory");
}


ALWAYS_INLINE
void set_ASID(uint32_t asid) {
	__asm__ __volatile("mcr p15, 0, %0, c13, c0, 1" :: "r" (asid) : "memory");
}

ALWAYS_INLINE
void enable_mmu() {
	__asm__ __volatile(
			"mrc p15, 0, r1, c1, c0, 0\n"
			"orr r1, r1, #0x1\n"
			"mcr p15, 0, r1, c1, c0, 0\n"
			"isb\n" 
			::: "memory"); 
}


ALWAYS_INLINE
void disable_mmu() {
	__asm__ __volatile(
			"mrc p15, 0, r1, c1, c0, 0\n"
			"bic r1, r1, #0x1\n"
			"mcr p15, 0, r1, c1, c0, 0\n"
			"isb\n"
		   	::: "memory"); 
}

ALWAYS_INLINE
int get_mmu_enable() {
	int ret = 0;
	__asm__ __volatile(
			"mrc p15, 0, %0, c1, c0, 0\n"
			: "=r" (ret)
			:: "memory");

	ret &= 0x1;
	return ret;
}
			

ALWAYS_INLINE
void invalidate_tlb() {
	__asm__ __volatile(
	"mcr p15, 0, r1, c8, c7, 0\n"
	// Invalidate entire instruction TLB
	"mcr p15, 0, r1, c8, c5, 0\n"
	// Invalidate entire data TLB
	"mcr p15, 0, r1, c8, c6, 0\n"
	::: "memory");
}

#endif

#endif
