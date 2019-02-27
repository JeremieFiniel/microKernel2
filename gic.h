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
 * See document: IHI0048B (unless stated otherwise)
 *
 *    ARM Generic Interrupt Controller - Architecture version 2.0
 *    Architecture Specification
 *
 */

#ifndef GIC_H_
#define GIC_H_


#define CPSR_USR_MODE 0x10
#define CPSR_FIQ_MODE 0x11
#define CPSR_IRQ_MODE 0x12
#define CPSR_SVC_MODE 0x13
#define CPSR_ABT_MODE 0x17
#define CPSR_UND_MODE 0x1B
#define CPSR_SYS_MODE 0x1F

#define CPSR_IRQ_FLAG   0x80      /* when set, IRQs are disabled, at the core level */
#define CPSR_FIQ_FLAG   0x40      /* when set, FIQs are disabled, at the core level */


typedef uint32_t irq_id_t;
typedef uint32_t cpu_id_t;

/*
 * Enables IRQs, FIQs unchanged.
 *    No MODE change... probably in SYS_MODE.
 *    Mask to disable IRQ:  NO_IRQ =0x80
 *    Mask to disable FIQ:  NO_FIQ =0x40
 * Clearing the IRQ mask bit in CPSR.
 *
 */
ALWAYS_INLINE
void arm_enable_interrupts(void) {
  unsigned long temp;
  __asm__ __volatile__(
      "mrs %0, cpsr\n"
      "bic %0, %0, #0x80\n"  /* IRQs (CPSR_IRQ_FLAG), no FIQs (CPSR_FIQ_FLAG) */
      "msr cpsr_c, %0"
      : "=r" (temp)
      :
      : "memory");
}

/*
 * Disables IRQ/FIQ interrupts
 *    No MODE change... probably in SYS_MODE.
 *    Mask to disable IRQ:  NO_IRQ =0x80
 *    Mask to disable FIQ:  NO_FIQ =0x40
 * Setting the IRQ and FIQ mask bit in CPSR.
 * returns true if interrupts had been enabled before we disabled them
 */
ALWAYS_INLINE
int arm_disable_interrupts(void) {
  unsigned long old, temp;
  __asm__ __volatile__(
      "mrs %0, cpsr\n"
      "orr %1, %0, #(0x80 | 0x40)\n" /* No IRQs (CPSR_IRQ_FLAG), no FIQs (CPSR_FIQ_FLAG) */
      "msr cpsr_c, %1"
      : "=r" (old), "=r" (temp)
      :
      : "memory");
  return (old & 0x80) == 0;
}


/*
 * Generic Interrupt Controller
 *
 * See ARM Generic Interrupt Controller Architecture Specification 1.0
 * Also see DDI0407G, Cortex A9 MPCore Technical Reference Manual.
 *
 */

#define ARM_GIC_CTLR    0x0000    // GICC_CTLR RW 0x00000000 CPU Interface Control Register
#define ARM_GIC_PMR     0x0004    // GICC_PMR RW 0x00000000 Interrupt Priority Mask Register
#define ARM_GIC_BPR     0x0008    // GICC_BPR RW 0x0000000x a Binary Point Register
#define ARM_GIC_IAR     0x000C    // GICC_IAR RO 0x000003FF Interrupt Acknowledge Register
#define ARM_GIC_EOIR    0x0010    // GICC_EOIR WO - End of Interrupt Register
#define ARM_GIC_RPR     0x0014    // GICC_RPR RO 0x000000FF Running Priority Register
#define ARM_GIC_HPPIR   0x0018    // GICC_HPPIR RO 0x000003FF Highest Priority Pending Interrupt Register
#define ARM_GIC_ABPR    0x001C    // GICC_ABPR b RW 0x0000000x a Aliased Binary Point Register
#define ARM_GIC_AIAR    0x0020    // GICC_AIAR c RO 0x000003FF Aliased Interrupt Acknowledge Register
#define ARM_GIC_AEOIR   0x0024    // GICC_AEOIR c WO - Aliased End of Interrupt Register
#define ARM_GIC_AHPPIR  0x0028    // GICC_AHPPIR c RO 0x000003FF Aliased Highest Priority Pending Interrupt Register
// 0x002C - 0x003C - - - Reserved
// 0x0040-0x00CF   ----  Implementation Defined Registers
#define ARM_GIC_APRn    0x00D0    // GICC_APRn c RW 0x00000000 Active Priorities Registers (0x00D0 - 0x00DC)
#define ARM_GIC_NSAPRn  0x00E0    // GICC_NSAPRn c RW 0x00000000 Non-secure Active Priorities Registers (0x00E0 - 0x00EC)
// 0x00ED - 0x00F8 - - - Reserved
#define ARM_GIC_IIDR    0x00FC    // GICC_IIDR RO IMPLEMENTATION DEFINED CPU Interface Identification Register
#define ARM_GIC_DIR     0x1000    // GICC_DIR c WO - Deactivate Interrupt Register

/*
 * GIC - Interface Control Register (ARM_GIC_CTLR)
 * Version 1:
 * [2] ACKCtl
 *        When the highest priority pending interrupt is a Group 1 interrupt,
 *        determines both:
 *           • whether a read of GICC_IAR acknowledges the interrupt,
 *             or returns a spurious interrupt ID
 *           • whether a read of GICC_HPPIR returns the ID of
 *             the highest priority pending interrupt, or returns a spurious interrupt ID.
 *
 *        0  If the highest priority pending interrupt is a Group 1 interrupt,
 *           a read of the GICC_IAR or the GICC_HPPIR returns an Interrupt ID of 1022.
 *           A read of the GICC_IAR does not acknowledge the interrupt, and has no effect
 *           on the pending status of the interrupt.
 *        1  If the highest priority pending interrupt is a Group 1 interrupt,
 *           a read of the GICC_IAR or the GICC_HPPIR returns the Interrupt ID of the Group 1 interrupt.
 *           A read of GICC_IAR acknowledges and Activates the interrupt.
 *
 * [1] EnableGrp1
 *         Enable for the signaling of Group 1 interrupts by the CPU interface to the connected processor.
 *         0 Disable signaling of interrupts
 *         1 Enable signaling of interrupts.
 *
 * [0] EnableGrp0
 *         Enable for the signaling of Group 0 interrupts by the CPU interface to the connected processor.
 *         0 Disable signaling of interrupts
 *         1 Enable signaling of interrupts.
 *
 * Version 2:
 * [31:10] Reserved
 * [9]     EOImodeNS Controls the behavior of Non-secure accesses to the GICC_EOIR and GICC_DIR registers:
 *         0  GICC_EOIR has both priority drop and deactivate interrupt functionality.
 *            Accesses to the GICC_DIR are UNPREDICTABLE .
 *         1  GICC_EOIR has priority drop functionality only. The GICC_DIR register has
 *            deactivate interrupt functionality.
 *            See Behavior of writes to GICC_EOIR, GICv2 on page 4-140 for more information.
 * [8:7]   Reserved.
 * [6]     IRQBypDisGrp1
 *            When the signaling of IRQs by the CPU interface is disabled,
 *            this bit partly controls whether the bypass IRQ signal is signaled to the processor:
 *         0 Bypass IRQ signal is signaled to the processor
 *         1 Bypass IRQ signal is not signaled to the processor.
 *
 *         See Interrupt signal bypass, and GICv2 bypass disable on page 2-27 for more information.
 *
 * [5]     FIQBypDisGrp1
 *         When the signaling of FIQs by the CPU interface is disabled, this bit partly controls
 *         whether the bypass FIQ signal is signaled to the processor:
 *         0 Bypass FIQ signal is signaled to the processor
 *         1 Bypass FIQ signal is not signaled to the processor.
 *         See Interrupt signal bypass, and GICv2 bypass disable on page 2-27 for more information.
 * [4:1]   Reserved
 * [0]     EnableGrp1
 *         Enable for the signaling of Group 1 interrupts by the CPU interface to the connected processor.
 *         0 Disable signaling of interrupts
 *         1 Enable signaling of interrupts.
 */

#define ARM_GIC_CTLR_ACKCTL (1<<2)
#define ARM_GIC_CTLR_GRP1   (1<<1)
#define ARM_GIC_CTLR_GRP0   (1<<0)
#define ARM_GIC_CTLR_EOImodeNS   (1<<9)

/*
 * Section 3.3 Interrupt prioritization
 * Page 44
 *
 * The lowest priority has the largest value (0xFF).
 * A smaller value indicates a higher priority.
 * The reset value of zero mask all interrupts.
 *
 * Certain implementations may choose to support less than 256 priority levels.
 * In this case, the behavior is really weird: the unsupported bits are the
 * lowest bits, not the highest. So priority levels are no longer contiguous,
 * but separated by a power-of-two step (2,4,8):
 *
 * Implemented priority bits    Possible priority field values            Number of priority levels
 * [7:0]                        0x00 - 0xFF (0-255), all values           256
 * [7:1]                        0x00 - 0xFE , (0-254), even values only   128
 * [7:2]                        0x00 - 0xFC (0-252), in steps of 4        64
 * [7:3]                        0x00 - 0xF8 (0-248), in steps of 8        32
 * [7:4]                        0x00 - 0xF0 (0-240), in steps of 16       16
 *
 */
#define ARM_GIC_PMR_LOWEST_PRIORITY 0xFF

/* ARM_GIC_BPR     0x0008
 *
 * See Section 3.3.3 page 45.
 *
 * GICC_BPR RW 0x0000000x a Binary Point Register
 * Priority grouping uses the Binary Point Register, GICC_BPR,
 * to split a priority value into two fields, the group priority and the subpriority.
 *
 * Table 3-2 Priority grouping by binary point
 * Binary point value       Group priority field      Subpriority field       Field with binary point
 * 0                        [7:1]                     [0]                     ggggggg.s
 * 1                        [7:2]                     [1:0]                   gggggg.ss
 * 2                        [7:3]                     [2:0]                   ggggg.sss
 * 3                        [7:4]                     [3:0]                   gggg.ssss
 * 4                        [7:5]                     [4:0]                   ggg.sssss
 * 5                        [7:6]                     [5:0]                   gg.ssssss
 * 6                        [7]                       [6:0]                   g.sssssss
 * 7                        No preemption             [7:0]                   .ssssssss
 *
 * Nota Bene: the minimum value for the BPR register is in the range 0-3,
 *            and it is implementation dependent.
*/

/*
 * ARM_GIC_IIDR
 * CPU Interface Implementer Identification Register
 * From Cortex-a9 MPCore Technical Reference Manual
 * Section 3.4.1, page 60
 *
 * Bits     Values  Name                  Function
 * [31:20]  0x390   Part number           Identifies the peripheral.
 * [19:16]  0x1     Architecture version  Identifies the architecture version.
 * [15:12]  0x2     Revision number       Returns the revision number of the Interrupt Controller.
 *                                        The implementer defines the format of this field.
 * [11:0]   0x43B   Implementer           Returns the JEP106 code of the company that implemented
 *                                        the Cortex-A9 processor interface RTL.
 *                                        It uses the following construct:
 *                                        [11:8] the JEP106 continuation code of the implementer
 *                                        [7]    0
 *                                        [6:0]  the JEP106 code [6:0] of the implementer.
 */

/*
 * Interrupt Acknowledge Register (ARM_GIC_IAR)
 *
 * The processor reads this register to obtain the interrupt ID of the signaled interrupt.
 * This read acts as an acknowledge for the interrupt.
 *
 * A read of the GICC_IAR returns the interrupt ID of the highest priority pending interrupt
 * for the CPU interface.
 *
 * The read returns a spurious interrupt ID of 1023 if any of the following apply:
 *    • forwarding of interrupts by the Distributor to the CPU interface is disabled
 *    • signaling of interrupts by the CPU interface to the connected processor is disabled
 *    • no pending interrupt on the CPU interface has sufficient priority for the interface
 *      to signal it to the processor.
 *
 * When GICC_CTLR.AckCtl is set to 0 in a GICv2 implementation that does not include
 * the Security Extensions, if the highest priority pending interrupt is in Group 1,
 * the interrupt ID 1022 is returned.
 *
 * [31:13] Reserved.
 * [12:10] CPUID For SGIs in a multiprocessor implementation,
 *         this field identifies the processor that requested the interrupt.
 *         It returns the number of the CPU interface that made the request,
 *         for example a value of 3 means the request was generated by a write
 *         to the GICD_SGIR on CPU interface 3.
 *         For all other interrupts this field is RAZ.
 * [9:0]   Interrupt ID The interrupt ID.
 */

#define ARM_GIC_IAR_SPURIOUS_GROUP1_IRQ   0x3FE
#define ARM_GIC_IAR_SPURIOUS_IRQ          0x3FF

#define ARM_GIC_IAR_SPURIOUS(irq) ((irq==0x3FE)||(irq==0x3FF))

#define ARM_GIC_ACK_INTID_MASK      0x000003FF /**< Interrupt ID */
#define ARM_GIC_ACK_INTID_OFF                0 /**< Interrupt ID */
#define ARM_GIC_CPUID_MASK          0x00000C00 /**< CPU ID */
#define ARM_GIC_CPUID_OFF                   10 /**< CPU ID */

void cortex_a9_gic_init(void);
void cortex_a9_gic_get_current_irq(irq_id_t *irq, cpu_id_t *src);
void cortex_a9_gic_acknowledge_irq(irq_id_t irq, cpu_id_t src);

void cortex_a9_gic_dump_state(void);


#endif /* GIC_H_ */
