/*
 * Copyright (C) Pr. Olivier Gruber. < olivier dot gruber at acm dot org >
 *
 * This code is part of a suite of educational software,
 * it is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Classpath; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 */

#ifndef BOARD_H_
#define BOARD_H_

/*
 * These includes are for the default type declarations,
 * such as int8_t or uint32_t.
 */
#include <stddef.h>
#include <stdint.h>

typedef uint8_t boolean_t;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/*
 * These macros are for aligning integer values on 16bit, 32bit, or 64bit boundaries.
 * For example:
 *
 *    ALIGN16(1) = 2
 *    ALIGN32(10) = 12
 */
#define ALIGN16_GRAIN (sizeof(uint16_t)-1)
#define ALIGN16(v) ((((uintptr_t)(v))+ALIGN16_GRAIN)& ~ALIGN16_GRAIN)

#define ALIGN32_GRAIN (sizeof(uint32_t)-1)
#define ALIGN32(v) ((((uintptr_t)(v))+ALIGN32_GRAIN)& ~ALIGN32_GRAIN)

#define ALIGN64_GRAIN (sizeof(uint64_t)-1)
#define ALIGN64(v) ((((uintptr_t)(v))+ALIGN64_GRAIN)& ~ALIGN64_GRAIN)


/**
 * This is to control the inlining of some functions.
 * This is to force inlining, always, and remove any overhead by asking
 * that GCC does not instrument the inlined function. The intent is to
 * obtain, with optimized code, a code as efficient as if the code was
 * written without any function call.
 */
#define ALWAYS_INLINE static __inline __attribute__((always_inline, no_instrument_function))

/**
 * This macro is to handle pointer shifts, from pointers to a field of a structure
 * back out to a pointer to the structure itself.
 * This macro is necessary because C compilers may or may not move fields around
 * in a C structure. Indeed, the C specification does not mandate that a C compiler
 * keeps the ordering of the fields of a structure.
 */
#define container_of(addr,field,type) \
  (void*)((uintptr_t)addr - __builtin_offsetof(type, field))

/*
 * Cortex-A9 MPCore Technical Reference Manual:
 * 1.5 Private Memory Regions (page 17)
 *
 * All registers accessible by all Cortex-A9 processors within the Cortex-A9 MPCore are grouped
 * into two contiguous 4KB pages accessed through a dedicated internal bus. The base address of
 * these pages is defined by the pins PERIPHBASE[31:13]. See Configuration signals on
 * page A-5 for more information on PERIPHBASE[31:13].
 *
 * Cortex-A9 MPCore global control and peripherals must be accessed through memory-mapped
 * transfers to the Cortex-A9 MPCore private memory region.
 * Memory regions used for these registers must be marked as Device or Strongly-ordered in the
 * translation tables.
 *
 * Access to the private memory region is little-endian only.
 * Access these registers with single load/store instructions. Load or store multiple accesses cause
 * an abort to the requesting Cortex-A9 processor and the Fault Status Register shows this as a
 * SLVERR
 *
 * Offset from PERIPHBASE[31:13] Peripheral Description
 *
 * 0x0000 - 0x00FC SCU registers Chapter 2 Snoop Control Unit
 * 0x0100 - 0x01FF Interrupt controller interfaces Chapter 3 Interrupt Controller
 * 0x0200 - 0x02FF Global timer About the Global Timer on page 4-8
 * 0x0300 - 0x03FF - -
 * 0x0400 - 0x04FF - -
 * 0x0500 - 0x05FF - -
 * 0x0600 - 0x06FF Private timers and watchdogs Private timer and watchdog registers on page 4-3
 * 0x0700 - 0x07FF Reserved Any access to this region causes a SLVERR abort exception
 * 0x0800 - 0x08FF
 * 0x0900 - 0x09FF
 * 0x0A00 - 0x0AFF
 * 0x0B00 - 0x0FFF
 * 0x1000 - 0x1FFF Interrupt Distributor interrupt sources on page 3-2
*/

#define ARM_GIC_BASE_OFFSET   0x00100  // Generic Interrupt Controller
#define ARM_GST_BASE_OFFSET   0x00200  // Global System Timer
#define ARM_PWT_BASE_OFFSET   0x00600  // Private Watchd dogs and Timers
#define ARM_GID_BASE_OFFSET   0x01000  // Generic Interrupt Distributor

#define CORTEX_A9_NIRQS           96

/*
 * Configuration Base Address Register takes the physical base address value at reset.
 *    - In Cortex-A9 uniprocessor implementations the base address is set to zero.
 *    - In Cortex-A9 MPCore implementations, the base address is reset to
 *      PERIPHBASE[31:13] so that software can determine the location of the
 *      private memory region.
 *
 * Examples:
 *    PERIPHBASE[31:13] = 0xF8F0_0000 (Zybo board)
 *    PERIPHBASE[31:13] = 0x1E00_0000 (Versatile Express board)
 */

ALWAYS_INLINE
uintptr_t cortex_a9_peripheral_base() {
  uintptr_t base;
  __asm volatile ("mrc p15, 4, %0, c15, c0, 0" : "=r" (base) : : "memory");
  return (base & ~((1<<13)-1));
}

#ifdef versatilepb
#define UART0 ((void*)0x101f1000)
#define UART0_IRQ 12
#define UART1 ((void*)0x101f2000)
#define UART1_IRQ 13
#define UART2 ((void*)0x101f3000)
#define UART2_IRQ 14
#endif

#ifdef vexpress_a9
/*
 * ARM CoreTile Express A9×4, Cortex-A9 MPCore (V2P-CA9)
 * Technical Reference Manual
 * Page 31, Section 2.6.2
 */

/*
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0447j/DUI0447J_v2m_p1_trm.pdf
 * For the memory map and interrupts.
 * Note that QEMU emulates the legacy memory map even with Cortex-A9.
 * ARM legacy memory map: the registers map onto the CS7 chip select.
 * SMB CS7: 0x1000-0000 - 0x1002-0000
 * Daughter board: 0x6000-0000 - 0xE0000-0000
 */
#define UART0 ((void*)0x10009000)
#define UART0_IRQ (32+5)
#define UART1 ((void*)0x1000a000)
#define UART1_IRQ (32+6)
#define UART2 ((void*)0x1000b000)
#define UART2_IRQ (32+7)
#define UART3 ((void*)0x1000c000)
#define UART3_IRQ (32+8)

#define TIMER_IRQ 29

#endif

extern void _arm_halt(void);

ALWAYS_INLINE
uint32_t armv7_coreid(void) {
   register uint32_t id = 0;
   __asm__ volatile (
       "mrc p15,0,%0,c0,c0,5"
      : "=r"(id)
      :
      );
   return (id & 0x3);
}

/*
 * Write operations.
 */
#define arm_mmio_write8(base,off, value)                    \
  *((volatile uint8_t *)((uintptr_t)(base)+((int32_t)off))) = (uint8_t)(value)

#define arm_mmio_write16(base,off,value)                     \
  *((volatile uint16_t *)((uintptr_t)(base)+((int32_t)off))) = (uint16_t)(value)

#define arm_mmio_write32(base,off,value)                     \
  *((volatile uint32_t *)((uintptr_t)(base)+((int32_t)off))) = (uint32_t)(value)

/*
 * Read operations.
 */
#define arm_mmio_read8(base,off)                       \
  (*((volatile uint8_t *)((uintptr_t)(base)+((int32_t)off))))

#define arm_mmio_read16(base,off)                      \
  (*((volatile uint16_t *)((uintptr_t)(base)+((int32_t)off))))

#define arm_mmio_read32(base,off)                      \
  (*((volatile uint32_t *)((uintptr_t)(base)+((int32_t)off))))

ALWAYS_INLINE void
_arm_mmio_setbits8(uintptr_t base, uint32_t off, uint8_t bits) {
  uint8_t val;
  val = arm_mmio_read8(base,off);
  val |= bits;
  arm_mmio_write8(base,off,val);
}
#define arm_mmio_setbits8(base,off,bits)                      \
    _arm_mmio_setbits8(((uintptr_t)(base)),((uint32_t)(off)),((uint8_t)(bits)))


ALWAYS_INLINE void
_arm_mmio_setbits16(uintptr_t base, uint32_t off, uint16_t bits) {
  uint16_t val;
  val = arm_mmio_read16(base,off);
  val |= bits;
  arm_mmio_write16(base,off,val);
}
#define arm_mmio_setbits16(base,off,bits)                      \
    _arm_mmio_setbits16(((uintptr_t)(base)),((uint32_t)(off)),((uint16_t)(bits)))

ALWAYS_INLINE void
_arm_mmio_setbits32(uintptr_t base, uint32_t off, uint32_t bits) {
  uint32_t val;
  val = arm_mmio_read32(base,off);
  val |= bits;
  arm_mmio_write32(base,off,val);
}
#define arm_mmio_setbits32(base,off,bits)                      \
    _arm_mmio_setbits32(((uintptr_t)(base)),((uint32_t)(off)),((uint32_t)(bits)))


ALWAYS_INLINE void
_arm_mmio_clearbits8(uintptr_t base, uint32_t off, uint8_t bits) {
  uint8_t val;
  val = arm_mmio_read8(base,off);
  val &= ~bits;
  arm_mmio_write8(base,off,val);
}
#define arm_mmio_clearbits8(base,off,bits)                      \
    _arm_mmio_clearbits8(((uintptr_t)(base)),((uint32_t)(off)),((uint8_t)(bits)))

ALWAYS_INLINE void
_arm_mmio_clearbits16(uintptr_t base, uint32_t off, uint16_t bits) {
  uint16_t val;
  val = arm_mmio_read16(base,off);
  val &= ~bits;
  arm_mmio_write16(base,off,val);
}
#define arm_mmio_clearbits16(base,off,bits)                      \
    _arm_mmio_clearbits16(((uintptr_t)(base)),((uint32_t)(off)),((uint16_t)(bits)))

ALWAYS_INLINE void
_arm_mmio_clearbits32(uintptr_t base, uint32_t off, uint32_t bits) {
  uint32_t val;
  val = arm_mmio_read32(base,off);
  val &= ~bits;
  arm_mmio_write32(base,off,val);
}
#define arm_mmio_clearbits32(base,off,bits)                      \
    _arm_mmio_clearbits32(((uintptr_t)(base)),((uint32_t)(off)),((uint32_t)(bits)))

ALWAYS_INLINE void
_arm_mmio_clearsetbits8(uintptr_t base, uint32_t off, uint8_t mask, uint8_t bits) {
  uint8_t val;
  val = arm_mmio_read8(base,off);
  val &= mask;
  val |= bits;
  arm_mmio_write8(base,off,val);
}
#define arm_mmio_clearsetbits8(base,off,mask,bits)                      \
    _arm_mmio_setbits8(((uintptr_t)(base)),((uint32_t)(off)),((uint8_t)(mask)),((uint8_t)(bits)))


ALWAYS_INLINE void
_arm_mmio_clearsetbits16(uintptr_t base, uint32_t off, uint16_t mask, uint16_t bits) {
  uint16_t val;
  val = arm_mmio_read16(base,off);
  val &= mask;
  val |= bits;
  arm_mmio_write16(base,off,val);
}
#define arm_mmio_clearsetbits16(base,off,mask,bits)                      \
    _arm_mmio_setbits16(((uintptr_t)(base)),((uint32_t)(off)),((uint16_t)(mask)),((uint16_t)(bits)))

ALWAYS_INLINE void
_arm_mmio_clearsetbits32(uintptr_t base, uint32_t off, uint32_t mask, uint32_t bits) {
  uint32_t val;
  val = arm_mmio_read32(base,off);
  val &= mask;
  val |= bits;
  arm_mmio_write32(base,off,val);
}
#define arm_mmio_clearsetbits32(base,off,mask,bits)                      \
    _arm_mmio_clearsetbits32(((uintptr_t)(base)),((uint32_t)(off)),((uint32_t)(mask)),((uint32_t)(bits)))


void kprintf(const char *fmt, ...);


/**
 * Assert: the intent is to check for possible abnormal conditions
 * that should result in a panic failure.
 */
#define assert(cond,format, ...) \
  do {\
    if (!(cond)) { \
      kprintf ("ASSERT: %s:%d\n",__FILE__, __LINE__); \
      kprintf (format, ##__VA_ARGS__); kprintf("\n"); \
      _arm_halt(); } \
  } while(0)

#define panic(code, format, ...) \
    do { \
      kprintf("PANIC: code=%d msg=",code); \
      kprintf(format, ##__VA_ARGS__); \
      kprintf("\n"); \
      _arm_halt(); \
    } while(0)


#endif /* BOARD_H_ */
