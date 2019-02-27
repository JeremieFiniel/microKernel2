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

#ifndef ARM_CORTEX_A9_GID_H_
#define ARM_CORTEX_A9_GID_H_

#define ARM_GID_IRQ_OFF(irq_no)              \
  ((irq_no >> 5) << 2)
#define ARM_GID_IRQ_VAL(irq_no)              \
  (1 << (irq_no % 32))

#define ARM_GID_IRQ_OFF8(irq_no)              \
  (irq_no >> 3)
#define ARM_GID_IRQ_VAL8(irq_no)              \
  (1 << (irq_no % 8))

/*
 * Generic Interrupt Distributor
 *
 * See ARM Generic Interrupt Controller Architecture Specification 1.0
 * Also see DDI0407G, Cortex A9 MPCore Technical Reference Manual.
 *
 */

#define ARM_GID_ICDDCR          0x000   // Distributor Control Register (RW), alias GICD_CTLR
#define ARM_GID_ICDICTR         0x004   // Interrupt Controller Type Register (RO)
#define ARM_GID_ICDIIDR         0x008   // Distributor Implementer Identification Register (RO)
#define ARM_GID_ICDISRn         0x080   // Interrupt Security Registers (RW,0x80-0x9c)
#define ARM_GID_ICDISERn        0x100   // Interrupt Set-Enable Registers (RW, 0x100 - 0x17C)
#define ARM_GID_ICDICERn        0x180   // Interrupt Clear-Enable Registers (RW, 0x180 - 0x1FC)

#define ARM_GID_ICDISPRn        0x200   // Interrupt Set-Pending Registers (RW, 0x200-0x27C)
#define ARM_GID_ICDICPRn        0x280   // Interrupt Clear-Pending Registers (RW,0x280-0x29C)
#define ARM_GID_ICDABRn         0x300   // Active Bit registers (RO,0x300-0x31C)

#define ARM_GID_ISACTIVERn      0x300   // GICv2 Interrupt Set-Active Registers (RW,0x300-0x37C)
#define ARM_GID_ICACTIVERn      0x380   // GICv2 Interrupt Clear-Active Registers (RW,0x380-0x3FC)

#define ARM_GID_ICDIPRn         0x400   // Interrupt Priority Registers (RW,0x400-4FC)
                                        // Reset value = 0x00000000
                                        // Usage constraints: these registers are byte-accessible.

#define ARM_GID_ICDIPTRn        0x800   // Interrupt Processor Targets Registers (RW,0x800-0x8FC)

#define ARM_GID_ICDICFRn        0xC00   // Interrupt Configuration Register (RW  0xC00-0xC3C)
#define ARM_GID_ICDICFR0        0xC00   // reset value = 0xAAAAAAAA
#define ARM_GID_ICDICFR1        0xC04   // reset value = 0x7DC00000
/*
 * ARM_GID_ICDICFRx        0xC04   // reset value = 0x55555555
 */

#define ARM_GID_ICPPISR         0xD00   // PPI Status Register (RO)
#define ARM_GID_ICSPISRn        0xD04   // SPI Status Registers (RO,0xD04-0xD1C)

#define ARM_GID_NSACRn        0xE00 /*-0xEFC*/

#define ARM_GID_ICDSGIR         0xF00   // Software Generated Interrupt Register (WO)
#define ARM_GID_ICPIDRn         0xFD0   // Peripheral ID[0:7] register (RO,0xFD0-0xFEC)
#define ARM_GID_ICCIDR0         0xFF0   // Component ID[0:3] register (RO,0xFF0-0xFFC)

// #define ARM_GID_CPENDSGIRn    0xF10 /*-0xF1C*/
// #define ARM_GID_SPENDSGIRn    0xF20 /*-0xF2C*/

/*
 * ARM_GID_ICDDCR (GICD_CTLR) fields
 * [31:2] - Reserved.
 * [1] EnableGrp1
 *     Global enable for forwarding pending Group 1 interrupts from the Distributor to the CPU interfaces:
 *     0 Group 1 interrupts not forwarded.
 *     1 Group 1 interrupts forwarded, subject to the priority rules.
 * [0] EnableGrp0
 *     Global enable for forwarding pending Group 1 interrupts from the Distributor to the CPU interfaces:
 *     0 Group 1 interrupts not forwarded.
 *     1 Group 1 interrupts forwarded, subject to the priority rules.
 */
#define ARM_GID_CTRL_ENABLE_GRP0  (1<<0)
#define ARM_GID_CTRL_ENABLE_GRP1  (1<<1)

/*
 * ARM_GID_TYPER fields
 * 31:16] - Reserved.
 * [15:11] LSPI
 *          If the GIC implements the Security Extensions, the value of this field is
 *          the maximum number of implemented lockable SPIs, from 0 ( 0b00000 ) to 31 ( 0b11111 ),
 *          see Configuration lockdown on page 4-82. (ARM GIC Architecture Reference)
 *          If this field is 0b00000 then the GIC does not implement configuration lockdown.
 *          If the GIC does not implement the Security Extensions, this field is reserved.
 * [10] SecurityExtn
 *          Indicates whether the GIC implements the Security Extensions.
 *          0 Security Extensions not implemented.
 *          1 Security Extensions implemented.
 * [9:8] - Reserved.
 * [7:5] CPUNumber
 *        Indicates the number of implemented CPU interfaces.
 *        The number of implemented CPU interfaces is one more than the value of this field,
 *        for example if this field is 0b011 , there are four CPU interfaces.
 *        If the GIC implements the Virtualization Extensions, this is also the number of
 *        virtual CPU interfaces.
 * [4:0] ITLinesNumber
 *        Indicates the maximum number of interrupts that the GIC supports. If ITLinesNumber=N,
 *        the maximum number of interrupts is 32(N+1).
 *        The interrupt ID range is from 0 to (number of IDs – 1).
 *        For example: 0b00011 Up to 128 interrupt lines, interrupt IDs 0-127.
 *        The maximum number of interrupts is 1020 ( 0b11111 ).
 *        See the text in this section (4.3.2 Interrupt Controller Type Register, GICD_TYPER)
 *        in the ARM GIC Architecture Reference Manual) for more information.
 *        Regardless of the range of interrupt IDs defined by this field,
 *        interrupt IDs 1020-1023 are reserved for special purposes,
 *        see Special interrupt numbers on page 3-43 and Interrupt IDs on page 2-24.
 */
#define ARM_GID_TYPER_LSPI        0x0000F800
#define ARM_GID_TYPER_SECUR_EXT   0x00000400
#define ARM_GID_TYPER_CPU_NB      0x000000E0
#define ARM_GID_TYPER_IT_LINES_NB 0x0000001F

/*
 * ARM_GID_ICDIIDR fields
 * Cortex-a9 MPCore, Technical Reference Manual
 * Section 3.3.3, page 56
 * [31:24] 0x01 Implementation version Gives implementation version number.
 * [23:12] 0x020 Revision number Returns the revision number of the controller.
 * [11:0] 0x43B Implementer Implementer number.
 */

#define ARM_GID_ICDIIDR_PRODUCTID    0xFF000000
#define ARM_GID_ICDIIDR_REVISION     0x00FFF000
#define ARM_GID_ICDIIDR_IMPLEM       0x00000FFF

/*
 * ARM_GID_IIDR fields
 */

#define ARM_GID_IIDR_PRODUCTID    0xFF000000
#define ARM_GID_IIDR_VARIANT      0x000F0000
#define ARM_GID_IIDR_REVISION     0x0000F000
#define ARM_GID_IIDR_IMPLEM       0x00000FFF

/*
 * ARM_GID_SGIR fields
 */

#define ARM_GID_SGIR_TARGETLISTFILTER_MASK 0x03000000
#define ARM_GID_SGIR_TARGETLISTFILTER_OFF          24
#define ARM_GID_SGIR_CPUTARGETLIST_MASK    0x00FF0000
#define ARM_GID_SGIR_CPUTARGETLIST_OFF             16
#define ARM_GID_SGIR_NSATT_MASK            0x00008000
#define ARM_GID_SGIR_NSATT_OFF                     15
#define ARM_GID_SGIR_SGIINTID_MASK         0x0000000F
#define ARM_GID_SGIR_SGIINTID_OFF                   0

#define ARM_GID_SGIR_TARGETLISTFILTER_LIST       0x00000000
#define ARM_GID_SGIR_TARGETLISTFILTER_ALL_BUT_ME 0x01000000
#define ARM_GID_SGIR_TARGETLISTFILTER_ME         0x02000000

/*
 * To be continued
 */

int cortex_a9_gid_enabled_irq(irq_id_t irq);
void cortex_a9_gid_enable_irq(irq_id_t irq);
void cortex_a9_gid_disable_irq(irq_id_t irq);
void cortex_a9_gid_soft_irq(cpu_id_t dst, uint8_t sgi_id);
void cortex_a9_gid_init(void);
void cortex_a9_gid_dump_state(void);

#endif /* ARM_CORTEX_A9_GID_H_ */
