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

#ifndef PL190_H_
#define PL190_H_


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

#define PL190_BAR0 0x10140000
#define PL190_BAR1 0x10140100 // VectAddr
#define PL190_BAR2 0x10140200 // VectCntls 0x200-0c23c
#define PL190_BAR3 0x10140300 // Test registers
#define PL190_BAR4 0x10140FE0 // Identification registers


/*
  INTERRUPT SOURCES (Versatile Board)
[31] External interrupt from secondary controller
[30] External interrupt signal from RealView Logic Tile or PCI3 interrupt signal
[29] External interrupt signal from RealView Logic Tile or PCI2 interrupt signal
[28] External interrupt signal from RealView Logic Tile or PCI1 interrupt signal
[27] External interrupt signal from RealView Logic Tile or PCI0 interrupt signal
[26] External interrupt signal from RealView Logic Tile or USB interrupt signal
[25] External interrupt signal from RealView Logic Tile or ETHERNET interrupt signal
[24] External interrupt signal from RealView Logic Tile or AACI interrupt signal
[23] External interrupt signal from RealView Logic Tile or MCI1A interrupt signal
[22] External interrupt signal from RealView Logic Tile or MCI0A interrupt signal
[21] External interrupt signal from RealView Logic Tile or DiskOnChip interrupt signal
[20]           GND                                 Reserved
[19]           MBX                                Graphics processor on development chip
[18]           PWRFAIL                        Power failure from FPGA
[17]           DMA                                DMA controller in development chip
[16]           CLCD                               CLCD controller in development chip
[15]           SCI0                                 Smart Card interface in development chip
[14]           UART2                             UART2 on development chip
[13]           UART1                             UART1 on development chip
[12]           UART0                             UART0 on development chip
[11]           SSP                                   Synchronous serial port in development chip
[10]           RTC                                  Real time clock in development chip
[9]             GPIO3                              GPIO controller in development chip
[8]             GPIO2                              GPIO controller in development chip
[7]             GPIO1                              GPIO controller in development chip
[6]             GPIO0                              GPIO controller in development chip
[5]             Timer 2 or 3                      Timers on development chip
[4]             Timer 0 or 1                      Timers on development chip
[3]             Comms TX                       Debug communications transmit interrupt.
This interrupt indicates that the communications channel is available for the processor to
pass messages to the debugger.
[2]             Comms RX                       Debug communications receive interrupt.
This interrupt indicates to the processor that messages are available for the processor to read.
[1]             Software interrupt            Software interrupt.
Enabling and disabling the software interrupt is done with the Enable Set and Enable Clear Registers.
Triggering the interrupt however, is done from the Soft Interrupt Set register.
[0]             Watchdog                         Watchdog timer
*/

#define PL190_DMA_INTR 17
#define PL190_UART2_INTR 14
#define PL190_UART1_INTR 13
#define PL190_UART0_INTR 12

/*
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0224i/DUI0224I_realview_platform_baseboard_for_arm926ej_s_ug.pdf
 * Section 4.25, page 263
  DMA:
    15 UART0 Tx
    14 UART0 Rx
    13 UART1 Tx
    12 UART1 Rx
    11 UART2 Tx
    10 UART2 Rx
*/

#define PL190_TIMER3_INTR 5
#define PL190_TIMER2_INTR 5
#define PL190_TIMER1_INTR 4
#define PL190_TIMER0_INTR 4
#define PL190_SOFTWARE_INTR 1
#define PL190_WATCHDOG_INTR 0


/*
 * irq_status (PICIRQStatus): IRQ status register (read-only)
 * High bits indicate active IRQs.
 *
 * fiq_status (PICFIQStatus): FIQ status register (read-only)
 * High bits indicate active IRQs
 *
 * raw_intr (PICRawIntr): Raw interrupt status register (read-only)
 * provides the status of the source interrupts, and software interrupts,
 * to the interrupt controller.
 * High bits indicate interrupts that are active before masking.
 *
 * intr_select (PICIntSelect):
 * Interrupt select register (read/write)
 * Selects whether the corresponding interrupt source generates an FIQ or an IRQ interrupt.
 * Selects the type of interrupt for interrupt requests:
 *   1 = FIQ interrupt
 *   0 = IRQ interrupt
 *
 * intr_enable (PICIntEnable): Interrupt enable register (read/write)
 * enables the interrupt request lines, by masking the interrupt sources for the IRQ interrupt.
 *    1 = Interrupt enabled. Enables interrupt request to processor.
 *    0 = Interrupt disabled.
 * On reset, all interrupts are disabled.
 * A HIGH bit sets the corresponding bit in the VICINTENABLE Register.
 * A LOW bit has no effect.
 *
 * intr_enable_clear (PICIntEnClear): Interrupt enable clear register (write)
 * Clears bits in the VICIntEnable Register.
 * A HIGH bit clears the corresponding bit in the VICINTENABLE Register.
 * A LOW bit has no effect.
 *
 * soft_intr (PICSoftInt): Software interrupt register (read/write)
 * Generates software interrupts/
 * Setting a bit generates a software interrupt for the specific source interrupt
 * before interrupt masking.
 * A HIGH bit sets the corresponding bit in the VICSOFTINT Register.
 * A LOW bit has no effect.
 *
 * soft_intr_clear (PICSoftIntClear): Software interrupt clear register (write-only)
 * Clears bits in the VICSOFTINT Register.
 * A HIGH bit clears the corresponding bit in the VICSOFTINT Register.
 * A LOW bit has no effect.
 *
 * protection (PICProtection): Protection enable register (Read/write)
 * [31-1] undefined
 * [0] enables or disables protected register access.
 * When enabled, only privileged mode accesses, reads and writes,
 * can access the interrupt controller registers.
 * When disabled, both User mode and Privileged mode can access
 * the registers. This register is cleared on reset,
 * and can only be accessed in privileged mode.
 *
 * vectaddr (PICVectAddr): vector address register (Read/write)
 * Contains the address of the currently active ISR.
 * Any writes to this register clear the interrupt.
 * Nota Bene:
 *   Reading from this register provides the address of the ISR,
 *   and indicates to the priority hardware that the interrupt is being serviced.
 *   Writing to this register indicates to the priority hardware that the interrupt has been serviced.
 *   You must use this register as follows:
 *       - the ISR reads the VICVectAddr Register when an IRQ interrupt is generated
 *       - at the end of the ISR, the VICVectAddr Register is written to, to update the priority hardware.
 *   WARNING: reading from or writing to the register at other times can cause incorrect operation.
 *
 * defvectaddr (PICDefVectAddr): Default vector address register (read/write)
 * Contains the address of the default ISR handler.
 */

struct __attribute__ ((__packed__)) pl190 {
  volatile uint32_t irq_status;
  volatile uint32_t fiq_status;
  volatile uint32_t raw_intr;
  volatile uint32_t intr_select;
  volatile uint32_t intr_enable;
  volatile uint32_t intr_enable_clear;
  volatile uint32_t soft_intr;
  volatile uint32_t soft_intr_clear;
  volatile uint32_t protection;
  uint8_t unused[12];
  volatile uint32_t vectaddr;
  volatile uint32_t default_vector_address;
};

/*
 * The read/write VICVECTADDR[0-15] Registers span address locations 0x100-0x13C
 * from PL190_BAR0 (0x10140000). They contain the ISR vector addresses.
 */
struct __attribute__ ((__packed__)) pl190_vectaddr {
  volatile uint32_t isrs[16];
};

/*
 * The read/write VICVECTCNTL[0-15] Registers span address locations 0x200-0x23C
 * from PL190_BAR0 (0x10140000). They select the interrupt source for the vectored interrupt.
 * [31:6]   Read undefined. Write as zero.
 * [5]      Enables vector interrupt. This bit is cleared on reset.
 * [4:0]    Selects interrupt source. You can select any of the 32 interrupt sources
 */
struct __attribute__ ((__packed__)) pl190_vectcntls {
  volatile uint32_t srcs[16];
};

/*
 * Read/Write Test registers
 * PICITCR  (0x10140300),
 * PICITIP1 (0x10140304)
 * PICITIP2 (0x10140308)
 * PICITOP1 (0x1014030c)
 * PICITOP2 (0x10140310)
 */
struct __attribute__ ((__packed__)) pl190_test_registers {
  volatile uint32_t itcr;
  volatile uint32_t itip1;
  volatile uint32_t itip2;
  volatile uint32_t ittop1;
  volatile uint32_t ittop2;
};

/*
 * Identification registers from 0x0FE to 0xFFF from PL190_BAR0:
 *
 * PICPeriphID0, PICPeriphID1, PICPeriphID2, PICPeriphID3.
 * The read-only VICPeriphID0-3 Registers are four 8-bit registers, that span address locations 0xFE0-0xFEC
 * You can treat the registers conceptually as a single 32-bit register.
 * The read-only registers provide the following options for the peripheral:
 *   Part number [11:0]
 *      This identifies the peripheral. The VIC uses the three-digit product code 0x90
 *   Designer [19:12]
 *      This is the identification of the designer. ARM Limited is 0x41, ASCII A.
 *   Revision number [23:20]
 *      This is the revision number of the peripheral. The revision number starts from 0.
 *   Configuration [31:24]
 *      This is the configuration option of the peripheral. The configuration value is 0.
 * See http://infocenter.arm.com/help/topic/com.arm.doc.ddi0181e/DDI0181.pdf
 * for details, section 3.3.14, page 43.
 *
 * PICPCellID0, PICPCellID1, PICPCellID2, PICPCellID3
 * The read-only VICPCELLID0-3 Registers are four 8-bit registers that span address locations
 * 0xFF0-0xFFC. You can treat them conceptually as a single 32-bit register:
 *  PICPCellID0 -> [0,7]
 *  PICPCellID1 -> [8,15]
 *  PICPCellID2 -> [16,23]
 *  PICPCellID3 -> [24,31]
 *  PCellID = (PICPCellID3 << 24) | (PICPCellID2 << 16) | (PICPCellID1 << 8) | PICPCellID0
 * Use the register as a standard cross-peripheral identification system.
 */
struct __attribute__ ((__packed__)) pl190_ident_registers {
  volatile uint32_t periphid0;
  volatile uint32_t periphid1;
  volatile uint32_t periphid2;
  volatile uint32_t periphid3;
  volatile uint32_t pcellid0;
  volatile uint32_t pcellid1;
  volatile uint32_t pcellid2;
  volatile uint32_t pcellid3;
};

void vic_init(void);
void vic_enable_irq(uint32_t irqno, uint32_t isr);
uint32_t vic_isr();
void vic_ack();


#endif /* PL190_H_ */
