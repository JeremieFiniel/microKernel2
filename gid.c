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

#include "board.h"
#include "gic.h"
#include "gid.h"

#undef VERBOSE

static uint32_t periph_base = 0x00;

extern void _arm_halt(void);

void kprintf(const char *fmt, ...);

static inline
uint32_t gid_read_reg8(uint32_t off) {
  uint32_t base = periph_base + ARM_GID_BASE_OFFSET;
  return arm_mmio_read8(base, off);
}

ALWAYS_INLINE
uint32_t gid_read_reg(uint32_t off) {
  uint32_t base = periph_base + ARM_GID_BASE_OFFSET;
  return arm_mmio_read32(base, off);
}

ALWAYS_INLINE
void gid_write_reg(uint32_t off, uint32_t val){
  uint32_t base = periph_base + ARM_GID_BASE_OFFSET;
  arm_mmio_write32(base, off, val);
}

ALWAYS_INLINE
void gid_write_reg8(uint32_t off, uint32_t val){
  uint32_t base = periph_base + ARM_GID_BASE_OFFSET;
  arm_mmio_write8(base, off, val);
}

int
cortex_a9_gid_enabled_irq(irq_id_t irq){
  uint8_t bits = gid_read_reg8(ARM_GID_ICDISERn + ARM_GID_IRQ_OFF8(irq));
  return (bits & ARM_GID_IRQ_VAL8(irq));
}

void
cortex_a9_gid_enable_irq(irq_id_t irq){
  gid_write_reg8(ARM_GID_ICDISERn + ARM_GID_IRQ_OFF8(irq),
                        ARM_GID_IRQ_VAL8(irq));
}

void
cortex_a9_gid_disable_irq(irq_id_t irq){
  gid_write_reg(ARM_GID_ICDICERn + ARM_GID_IRQ_OFF(irq),
                        ARM_GID_IRQ_VAL(irq));
}



void
cortex_a9_gid_soft_irq(uint32_t targets, uint8_t sgi_id){
  uint32_t val =
    ((sgi_id << ARM_GID_SGIR_SGIINTID_OFF) & ARM_GID_SGIR_SGIINTID_MASK) |
    ((targets << ARM_GID_SGIR_CPUTARGETLIST_OFF) & ARM_GID_SGIR_CPUTARGETLIST_MASK) |
    ARM_GID_SGIR_TARGETLISTFILTER_LIST;

  gid_write_reg(ARM_GID_ICDSGIR, val);
}


/*
 * Cortex-a9 MPCore, Technical Reference Manual
 * Section 3.3.3, page 56
 * [31:24] 0x01 Implementation version Gives implementation version number.
 * [23:12] 0x020 Revision number Returns the revision number of the controller.
 * [11:0] 0x43B Implementer Implementer number.
 */
static void
cortex_a9_check_identification() {
  uint32_t flags = 0;
  flags = gid_read_reg(ARM_GID_ICDIIDR);
  uint32_t v1 = (uint32_t) ((flags>>24) & 0xFF);
  uint32_t v2 = (uint32_t)((flags>>12)&0x0FFF);
  uint32_t v3 = (uint32_t)((flags>>0)&0xFFF);

#ifdef VERBOSE
  kprintf("GID [0x01 0x20 0x43B]: 0x%x 0x%x 0x%x \n\r",
      v1,v2,v3);
  kprintf("GID: V1 Rev2 \n");
  flags = gid_read_reg(ARM_GID_ICDICTR);
  if (flags & ARM_GID_TYPER_SECUR_EXT) {
    uint32_t lspi = (flags & ARM_GID_TYPER_LSPI)>>11;
    kprintf(" + Security extensions, %d LSPI \n",lspi);
  }
  uint32_t ncpus = (flags & ARM_GID_TYPER_CPU_NB) >> 5;
  kprintf(" %d cpu interfaces \n",ncpus);

  uint32_t nirqs = (flags & ARM_GID_TYPER_IT_LINES_NB);
  kprintf(" %d IRQ lines\n",nirqs);
#endif
}

/*
 * Distributor initialisation
 */

#define XSCUGIC_INT_CFG_OFFSET_CALC(InterruptID) \
  (ARM_GID_ICDICFRn + ((InterruptID/16) * 4))

void
cortex_a9_gid_init(void){

  periph_base = cortex_a9_peripheral_base();
#ifdef VERBOSE
  kprintf("PERIPHERAL BASE: 0x%x \n\r",periph_base);
#endif
  cortex_a9_check_identification();

  /* Disable GIC Distributor */
  gid_write_reg(ARM_GID_ICDDCR, 0);

  /*
   * Interrupt Configuration Registers (ICDICFRn 0xC00-0xC3C)
   *
   * This section describes the implementation defined features of the ICDICFR.
   * Each bit-pair describes the interrupt configuration for an interrupt.
   * The options for each pair depend on the interrupt type as follows:
   *  SGI: The bits are read-only and a bit-pair always reads as b10.
   *  PPI: The bits are read-only
   *    PPI[1] and [4]:b01
   *      interrupt is active LOW level sensitive.
   *    PPI[0], [2],and[3]:b11
   *      interrupt is rising-edge sensitive.
   *  SPI:
   *    The LSB bit of a bit-pair is read-only and is always b1.
   *    You can program the MSB bit of the bit-pair to alter
   *    the triggering sensitivity as follows:
   *      b01 interrupt is active HIGH level sensitive
   *      b11 interrupt is rising-edge sensitive.
   *    There are 31 LSPIs, interrupts 32-62. You can configure and then lock these
   *    interrupts against more change using CFGSDISABLE. The LSPIs are present
   *    only if the SPIs are present.
   */
  /*
   * 1. The trigger mode in the int_config register
   *    Only write to the SPI interrupts, so start at 32
   *    Set them all to be level sensitive, active HIGH.
   */
  int irqno = 0;
  int offset =0;
  gid_write_reg(ARM_GID_ICDICFR0,0xAAAAAAAA);
  gid_write_reg(ARM_GID_ICDICFR1,0x7DC00000);
  irqno = 32;
  offset = 8;
  while (irqno < CORTEX_A9_NIRQS) {
    gid_write_reg8(ARM_GID_ICDICFRn + offset, 0x55);
    irqno+=4;
    offset++;
  }

  /*
   * Interrupt Priority Registers (RW,0x400-4FC)
   * The GICD_IPRIORITYRs provide an 8-bit priority field for each interrupt
   * supported by the GIC. This field stores the priority of the corresponding
   * interrupt. The GID_BPR = 0x02, so the group split of the priority over
   * the 8bit is the following: ggggg sss
   * Default priority: 0x88 (8bits)
   * Usage constraints: these registers are byte-accessible.
   */
  irqno = 0;
  offset = 0;
  while (irqno<CORTEX_A9_NIRQS) {
    gid_write_reg8(ARM_GID_ICDIPRn + offset, 0x88);
    irqno++;
    offset++;
  }
  /*
   * Interrupt Processor Targets Registers (RW,0x800-0x8FC)
   * One field per core (8bit fields).
   *    0bxxxxxxx1 CPU interface 0
   *    0bxxxxxx1x CPU interface 1
   *    etc.
   * Only write to the SPI interrupts, so start at 32
   * All SPI interrupts set up to target cpu0.
   */
  irqno = 32;
  offset = 32;
  while (irqno<CORTEX_A9_NIRQS) {
    gid_write_reg8(ARM_GID_ICDIPTRn + offset, 0x01);
    irqno++;
    offset++;
  }

  /*
   * Interrupt Clear-Enable Registers (RW, 0x180 - 0x1FC)
   * Disable all interrupts (1bit fields)
   */
  irqno = 0;
  offset = 0;
  while (irqno<CORTEX_A9_NIRQS) {
    gid_write_reg8(ARM_GID_ICDICERn + offset, 0xFF);
    irqno+=8;
    offset++;
  }
  /*
   * Clear all pending and active interrupts (1bit)
   * Interrupt Clear-Pending Registers (RW,0x280-0x29C)
   * Active Bit registers (RO,0x300-0x31C)
   */
  irqno = 0;
  offset = 0;
  while (irqno<CORTEX_A9_NIRQS) {
    gid_write_reg8(ARM_GID_ICACTIVERn + offset, 0xFF);
    gid_write_reg8(ARM_GID_ICDICPRn + offset, 0xFF);
    irqno+=8;
    offset++;
  }
  /*
   * Enable Group0 and Group1 on GIC Distributor
   */
  gid_write_reg(ARM_GID_ICDDCR, ARM_GID_CTRL_ENABLE_GRP1 | ARM_GID_CTRL_ENABLE_GRP0);

}

void
cortex_a9_gid_dump_state(void){

  kprintf("=========================================\n");

  kprintf("GID_ICDDCR:        0x%08x\n", gid_read_reg(ARM_GID_ICDDCR));
  kprintf("GID_ICDICTR:       0x%08x\n", gid_read_reg(ARM_GID_ICDICTR));

  uint32_t irqno;
  uint32_t off;
  kprintf("GID Interrupt Set enabled IRQs: (1bit-fields)\n");
  off = 0;
  irqno = 0;
  kprintf("  ICDISERn: ");
  while (irqno<CORTEX_A9_NIRQS) {
    kprintf("%02x ", gid_read_reg8(ARM_GID_ICDISERn + off));
    irqno+=8;
    off+=1;
  }
  kprintf("\n");
  kprintf("GID Interrupt Set-Pending Registers: (1bit-fields)\n");
  off = 0;
  irqno = 0;
  kprintf("  ICDISPRn: ");
  while (irqno<CORTEX_A9_NIRQS) {
    kprintf("%02x ", gid_read_reg8(ARM_GID_ICDISPRn + off));
    irqno+=8;
    off+=1;
  }
  kprintf("\n");
  kprintf("GID Active Bit Registers: (1bit-fields)\n");
  off = 0;
  irqno = 0;
  kprintf("  ICDABRn:  ");
  while (irqno<CORTEX_A9_NIRQS) {
    kprintf("%02x ", gid_read_reg8(ARM_GID_ICDABRn + off));
    irqno+=8;
    off+=1;
  }
  kprintf("\n");
  kprintf("GID Interrupt Priority Registers: (8bit-fields):\n");
  off = 0;
  irqno = 0;
  kprintf("  ICDIPRn[0]: ");
  while (irqno<CORTEX_A9_NIRQS) {
    for (int i=0;i<16;i++) {
      kprintf("%02x ", gid_read_reg8(ARM_GID_ICDIPRn + off));
      irqno++;
      off+=1;
    }
    if (irqno<CORTEX_A9_NIRQS)
      kprintf("\n  ICDIPRn[%d]: ",irqno);
    else
      kprintf("\n");
  }
  kprintf("\n");
  kprintf("GID Interrupt Configuration Registers: (2bit-fields)\n");
  off = 0;
  irqno = 0;
  kprintf("  ICDICFRn: ");
  while (irqno<CORTEX_A9_NIRQS) {
    kprintf("%02x ", gid_read_reg8(ARM_GID_ICDICFRn + off));
    irqno+=4;
    off+=1;
  }
  kprintf("\n\n");
  kprintf("GID Interrupt Processor Targets Registers: (8bit-fields)\n");
  off = 0;
  irqno = 0;
  kprintf("  ICDIPTRn[0]: ");
  while (irqno<CORTEX_A9_NIRQS) {
    for (int i=0;i<16;i++) {
      kprintf("%02x ", gid_read_reg8(ARM_GID_ICDIPTRn + off));
      irqno++;
      off+=1;
    }
    if (irqno<CORTEX_A9_NIRQS)
      kprintf("\n  ICDIPTRn[%d]: ",irqno);
    else
      kprintf("\n");
  }
  kprintf("\n");
  kprintf("GID Status Registers:\n");
  kprintf("  PPISR:       0x%08x\n", gid_read_reg(ARM_GID_ICPPISR));
  kprintf("  SPISR[0]:    0x%08x\n", gid_read_reg(ARM_GID_ICSPISRn + 0));
  kprintf("  SPISR[1]:    0x%08x\n", gid_read_reg(ARM_GID_ICSPISRn + 4));

}

/*
 * Local Variables:
 * mode: c
 * tab-width: 2
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

