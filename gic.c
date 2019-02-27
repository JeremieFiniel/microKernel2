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

static uint32_t periph_base = 0x00;

void kprintf(const char *fmt, ...);

ALWAYS_INLINE
uint32_t gic_read_reg(uint32_t off) {
  uint32_t base = periph_base + ARM_GIC_BASE_OFFSET;
  return arm_mmio_read32(base, off);
}

ALWAYS_INLINE
void gic_write_reg(uint32_t off, uint32_t val){
  uint32_t base = periph_base + ARM_GIC_BASE_OFFSET;
  arm_mmio_write32(base, off, val);
}

/*
 * Interrupt Acknowledge Register (ARM_GIC_IAR)
 *
 * A read of the GICC_IAR returns the raised interrupt, that is, the interrupt
 * currently relayed to the processor by the GIC. Therefore, reading the ARM_GIC_IAR
 * register is meaningful from the interrupt handler to determine which interrupt
 * was raised (relayed by the GIC to the processor).
 *
 * The relayed interrupt was one of the pending interrupts, the interrupts with the
 * corresponding IRQ line raised by the corresponding device. Furthermore, the relayed
 * interrupt is one with the highest priority amongst the pending interrupts
 * for the CPU interface.
 *
 * The read returns a spurious interrupt ID of 1023 if any of the following apply:
 *    • forwarding of interrupts by the Distributor to the CPU interface is disabled
 *    • signaling of interrupts by the CPU interface to the connected processor is disabled
 *    • no pending interrupt on the CPU interface has sufficient priority for the interface
 *      to signal it to the processor.
 *
 */
void
cortex_a9_gic_get_current_irq(irq_id_t *irq, cpu_id_t *src){
  uint32_t val = 0;
  val = gic_read_reg(ARM_GIC_IAR);
  *irq = (val & ARM_GIC_ACK_INTID_MASK) >> ARM_GIC_ACK_INTID_OFF;
  *src = (val & ARM_GIC_CPUID_MASK) >> ARM_GIC_CPUID_OFF;
  return;
}


/*
 * Acknowledge the raised interrupt, that is, from an interrupt handler.
 *
 * When the GIC relays an interrupt to the processor, it is in the pending state,
 * and was selected because it was the pending interrupt with the highest priority.
 * When the interrupt handler has read the Interrupt Acknowledge Register (ARM_GIC_IAR),
 * the GIC passed the interrupt from pending to active.
 *
 * This is about acknowledging the interrupt for the GIC, which means the interrupt will
 * be inactive and probably not pending. The interrupt may remain pending if the device
 * is still asserting its IRQ line high (for level-sensitive interrupts). This means that
 * the interrupt must be acknowledged at the device level first.
 *
 * The procedure to acknowledge an interrupt with the GIC differs depending on the GIC.CTLR
 * setup, specifically the GIC.CTLR.EOImode flag. Set it to 1 distinguishes the priority drop
 * and the interrupt deactivation, both necessary to acknowledge an interrupt.
 * If GIC.CTLR.EOImode == 0, writing to the GIC EOIR register will both induce the priority drop
 * and the interrupt deactivation, effectively acknowledging the interrupt in one operation.
 *
 * The processor reads this register to obtain the interrupt ID of the signaled interrupt.
 * This read acts as an acknowledge for the interrupt.
 */
void
cortex_a9_gic_acknowledge_irq(irq_id_t irq, cpu_id_t src){
  uint32_t val = 0;
  val |= (irq << ARM_GIC_ACK_INTID_OFF) & ARM_GIC_ACK_INTID_MASK;
  val |= (src << ARM_GIC_CPUID_OFF) & ARM_GIC_CPUID_MASK;
  /* WARNING: this single write requires GIC.CTLR.EOImode == 0 */
  gic_write_reg(ARM_GIC_EOIR, val);
  return;
}


/*
 * Cortex-a9 MPCore, Technical Reference Manual
 * Section 3.4.1, page 60
 * [31:20] 0x390 Part number            Identifies the peripheral.
 * [19:16] 0x1    Architecture version  Identifies the architecture version.
 * [15:12] 0x2    Revision number       Returns the revision number of the Interrupt Controller.
 *                                      The implementer defines the format of this field.
 * [11:0] 0x43B   Implementer           Returns the JEP106 code of the company that implemented the Cortex-A9
 *                                      processor interface RTL. It uses the following construct:
 *                                      [11:8] the JEP106 continuation code of the implementer
 *                                      [7]    0
 *                                      [6:0]  the JEP106 code [6:0] of the implementer.
 *
 *  For example, the Zybo board returns the following values:
 *    [31:20] 0x390
 *    [19:16] 0x1
 *    [15:12] 0x2
 *    [11:0]  0x43B
 *  Notice: QEMU often returns zeroes...
 */
static void
cortex_a9_check_identification() {
  int flags = 0;
  int val;
  flags = gic_read_reg(ARM_GIC_IIDR);

#ifdef VERBOSE
  kprintf("GIC [0x%x:0x%x.0x%x 0x%x]\n\r",
      ((flags>>20) & 0xFFF),
      ((flags>>16)&0x0F),
      ((flags>>12)&0x0F),
      ((flags>>0)&0xFFF));
#endif
}

/*
 * CPU interface initialization
 *
 * If the CPU operates in both security domains, set parameters in the
 * control_s register as follows:
 *    1. Set FIQen=1 to use FIQ for secure interrupts,
 *    2. Program the AckCtl bit
 *    3. Program the SBPR bit to select the binary pointer behavior
 *    4. Set EnableS = 1 to enable secure interrupts
 *    5. Set EnbleNS = 1 to enable non secure interrupts
 *
 * If the CPU operates only in the secure domain, setup the
 * control_s register as follows:
 *    1. Set FIQen=1,
 *    2. Set EnableS=1, to enable the CPU interface to signal secure interrupts.
 * Only enable the IRQ output unless secure interrupts are needed.
 */

void
cortex_a9_gic_init(void){

  periph_base = cortex_a9_peripheral_base();
#ifdef VERBOSE
  kprintf("PERIPHERAL BASE: 0x%x \n\r",periph_base);
#endif
  cortex_a9_check_identification();

  /*
   * Setup priority mask register to the smallest priority
   * in order to forward all interrupts from GIC interface to the processor
   */
  gic_write_reg(ARM_GIC_PMR, ARM_GIC_PMR_LOWEST_PRIORITY);

  /* Enable CPU interface */
  uint32_t flags;
  flags = ARM_GIC_CTLR_ACKCTL | ARM_GIC_CTLR_GRP1 | ARM_GIC_CTLR_GRP0;
  gic_write_reg(ARM_GIC_CTLR, flags);

}


void
cortex_a9_gic_dump_state(void){
  irq_id_t irq;
  cpu_id_t src;
  uint32_t val = 0;

  uintptr_t periph_base = cortex_a9_peripheral_base();
  uintptr_t gic_base = periph_base + ARM_GIC_BASE_OFFSET;

  kprintf("GIC: periph=0x%08x gic=0x%08x\n",periph_base,gic_base);

  val = gic_read_reg(ARM_GIC_CTLR);
  kprintf("  CTLR:    0x%08x\n", val);

  val = gic_read_reg(ARM_GIC_IAR);
  kprintf("  IAR:    0x%08x\n", val);

  irq = (val & ARM_GIC_ACK_INTID_MASK) >> ARM_GIC_ACK_INTID_OFF;
  src = (val & ARM_GIC_CPUID_MASK) >> ARM_GIC_CPUID_OFF;

  val = 0;
  val |= (irq << ARM_GIC_ACK_INTID_OFF) & ARM_GIC_ACK_INTID_MASK;
  val |= (src << ARM_GIC_CPUID_OFF) & ARM_GIC_CPUID_MASK;

  kprintf("  --> <%d> got irq=%d cpuid=%d (EOIR val : 0x%08x)\n", armv7_coreid(), irq, src, val);

  gic_write_reg(ARM_GIC_EOIR, val);

  kprintf("  EOIR:   0x%08x\n", gic_read_reg(ARM_GIC_EOIR));
  kprintf("  CTRL:   0x%08x\n", gic_read_reg(ARM_GIC_CTLR));
  kprintf("  PMR:    0x%08x\n", gic_read_reg(ARM_GIC_PMR));
  kprintf("  BPR:    0x%08x\n", gic_read_reg(ARM_GIC_BPR));
  kprintf("  RPR:  0x%08x\n", gic_read_reg(ARM_GIC_RPR));
  kprintf("  HPPIR:  0x%08x\n", gic_read_reg(ARM_GIC_HPPIR));
}

/*
 * Local Variables:
 * mode: c
 * tab-width: 2
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

