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

/*
 * Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs
 */

    .equ    CPSR_USR_MODE,       0x10
    .equ    CPSR_FIQ_MODE,       0x11
    .equ    CPSR_IRQ_MODE,       0x12
    .equ    CPSR_SVC_MODE,       0x13
    .equ    CPSR_ABT_MODE,       0x17
    .equ    CPSR_UND_MODE,       0x1B
    .equ    CPSR_SYS_MODE,       0x1F

    .equ    CPSR_IRQ_FLAG,         0x80      /* when set, IRQs are disabled, at the core level */
    .equ    CPSR_FIQ_FLAG,         0x40      /* when set, FIQs are disabled, at the core level */

	.text
    .code 32

/**
 * This is the function to halt the processor.
 * Notice that we put it in low-power mode,
 * after we disable the IRQs and FIQs.
 *
 * THIS IS A FUNCTION THAT NEVER RETURNS
 */
	.global _arm_halt
	.func _arm_halt
_arm_halt:
    // disable IRQs and FIQs
    mrs r0, cpsr
    orr r0, r0, #(CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
    msr cpsr_c, r0
    // go in low-power mode
	mcr p15,0,r0,c7,c0,4
	// in case we still wakeup, just loop over.
	b	_arm_halt
	.size  _arm_halt, . - _arm_halt
	.endfunc

/*
 * This is a sleep method, to be called from a scheduler,
 * when there is nothing to do. This code will wait for the
 * next IRQ or FIQ and then return (after the IRQ/FIQ handler
 * has been called and it has returned.
 */
	.global _arm_sleep
	.func _arm_sleep
_arm_sleep:
    // go in low-power mode, until next IRQ/FIQ
	mcr p15,0,r0,c7,c0,4
	// something woke us up, just return.
	mov pc,lr
	.size  _arm_sleep, . - _arm_sleep
	.endfunc


/**
 * This is the initialization necessary to turn on IRQs later.
 * This code does the following:
 *
 *   - Sets up the stacks for the different modes of the ARM processor
 *   - Sets the interrupt vector
 *   - Disable the MMU, enables caching.
 *
 * NOTA BENE: this code works for an ARMv5, such as arm926ej-s.
 */
	.global _arm_irq_init
	.func _arm_irq_init
_arm_irq_init:

	mov r0, lr  @ save the return address in a non-banked register.

	/*----------------------------------------------
     * Initialize stack pointers for all ARM modes
	 *----------------------------------------------*/
	MSR     CPSR_c,#(CPSR_IRQ_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
	LDR     sp,=_irq_stack_top    /* set the IRQ stack pointer */

	MSR     CPSR_c,#(CPSR_FIQ_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
	LDR     sp,=_fiq_stack_top    /* set the FIQ stack pointer */

	MSR     CPSR_c,#(CPSR_SVC_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
	LDR     sp,=_svc_stack_top    /* set the SVC stack pointer */

	MSR     CPSR_c,#(CPSR_ABT_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
	LDR     sp,=_abt_stack_top    /* set the ABT stack pointer */

	MSR     CPSR_c,#(CPSR_UND_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
	LDR     sp,=_und_stack_top    /* set the UND stack pointer */

	MSR     CPSR_c,#(CPSR_SYS_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
	LDR     sp,=_sys_stack_top    /* set the C stack pointer */

	mov lr, r0  @ restore the return address.

	/*-------------------------------------------
	 * Reset the exception vector
	 * Copy first the secondary vector table,
	 * and then the primary vector table.
	 *-------------------------------------------*/
	mrc p15, 0, r0, c1, c0, 0 @ fetch system control register (SCTLR)
	bic r1, r1, #(1<<13)      @ Force low interrupt vector
	mcr p15, 0, r0, c1, c0, 0 @ store system control register (SCTLR)

	ldr     r3,=_secondary
	mov	r4, #0x20
	mov	r9, #0x40
1:
	ldmia	r3!, {r5} // load multiple from r3
	stmia	r4!, {r5} // store multiple at r4
	cmp	r4, r9 // are we done yet?
	blo	1b

	ldr     r3,=_primary
	mov	r4, #0x00
	mov	r9, #0x20
1:
	ldmia	r3!, {r5} // load multiple from r3
	stmia	r4!, {r5} // store multiple at r4
	cmp	r4, r9 // are we done yet?
	blo	1b

	/*
	 * Disable MMU
	 */
	mrc p15, 0, r1, c1, c0, 0 @ Read Control Register configuration data
	bic r1, r1, #0x1          @ Disable MMU
    mcr p15, 0, r1, c1, c0, 0 @ Write Control Register configuration data

	/*
	 * If no MMU, we can still enable L1 Caches (I-Cache and D-Cache),
	 * write buffers, barriers, and branch prediction.
	 */
	mrc p15, 0, r1, c1, c0, 0  @ Read Control Register configuration data
	orr r1, r1, #(0x1 << 12)   @ enable I Cache
	orr r1, r1, #(0x1 << 2)    @ enable D Cache
	orr r1, r1, #(0x1 << 3)    @ enable Write buffer
	orr r1, r1, #(0x1 << 5)    @ enable Barriers
	orr r1, r1, #(0x1 << 11)   @ branch prediction
	mcr p15, 0, r1, c1, c0, 0  @ Write Control Register configuration data
	mcr p15, 0, r0, c7, c10, 5 @ data memory barrier

	mov pc, lr

	.size   _arm_irq_init, . - _arm_irq_init
	.endfunc

.p2align 8
_primary:
	ldr pc,[pc,#0x18] /* 0x00 reset */
	ldr pc,[pc,#0x18] /* 0x04 undefined instruction */
	ldr pc,[pc,#0x18] /* 0x08 software interrupt */
	ldr pc,[pc,#0x18] /* 0x0c prefetch abort */
	ldr pc,[pc,#0x18] /* 0x10 data abort */
	ldr pc,[pc,#0x18] /* 0x14 reserved */
	ldr pc,[pc,#0x18] /* 0x18 IRQ */
	ldr pc,[pc,#0x18] /* 0x1c FIQ */
_secondary:
	.word 0x00
	.word 0x04   /* 0x04  better than halt, for debug, because we loop through the ldr instruction */
	.word 0x08   /* 0x08  better than halt, for debug, because we loop through the ldr instruction */
	.word 0x0c   /* 0x0c  better than halt, for debug, because we loop through the ldr instruction */
	.word _arm_data_abort
	.word 0x14   /* 0x14  better than halt, for debug, because we loop through the ldr instruction */
	.word _arm_irq_handler
	.word 0x1c


/*
 * The EXT bit can provide classification of external aborts on some implementations,
 * while the WnR bit indicates whether the abort was on a data write (1) or data read (0).
 *
 * Bits [13]	CM
 *		[12]	EXT
 *		[11]	WnR
 *		[10]	FS[4]
 *		[9:8]	LPAE
 *		[7:4]	Domain
 *		[3:0]	FS[3:0]
 *
 * DFSR [10,3:0] Source DFAR
	00001 Alignment fault Valid
	00100 Instruction cache maintenance fault Valid
	01100 Page table walk synchronous external abort 1st level valid
	01100                                            2nd level Valid
	11100 Page table walk synchronous parity error 1st level   valid
	11100                                          2nd level   Valid
	00101 Translation fault Section    	Valid
	00111                   Page 		Valid
	00011 Access flag fault Section     Valid
	00110                   Page 		Valid
	01001 Domain fault 		Section		Valid
	01011 					Page 		Valid

	01101 Permission fault		Section 	Valid
	01111                       Page 		Valid
	00010 Debug event
	00001 Alignment fault 					Valid
	01000 Synchronous external abort 		Valid
	10100 IMPLEMENTATION DEFINED -
	11010 IMPLEMENTATION DEFINED -
	11001 Memory access synchronous parity error Valid
	10110 Asynchronous external abort
	11000 Memory access asynchronous parity error
*/

_arm_data_abort:
	/*
	 * Let's get at the reasons of the data abort
	 * DFSR: Data Fault Status Register
	 * DFAR: Data Fault Address Register
	 *
	 */
	mrc p15, 0, r0, c5, c0, 0 @ Read DFSR
	mrc p15, 0, r1, c6, c0, 0 @ Read DFAR

    /*
     * Switch back to sys mode so that we have access
     * to the C stack from the debugger.
     */
	msr cpsr_c,#(CPSR_SYS_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)

	/* Enter a forever loop */
1:
	b 1b


_arm_irq_handler:
    MOV     r13,r0              /* save r0 in r13_IRQ */
    SUB     r0,lr,#4            /* put return address in r0_SYS */
    MOV     lr,r1               /* save r1 in r14_IRQ (lr) */
    MRS     r1,spsr             /* put the SPSR in r1_SYS */

    MSR     cpsr_c,#((CPSR_SYS_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)) /* SYS mode, no IRQ/FIQ! */
    STMFD   sp!,{r0,r1}         /* save SPSR and PC on SYS stack */
    STMFD   sp!,{r2-r3,r12,lr}  /* save APCS-clobbered regs on SYS stack */
    MOV     r0,sp               /* make the sp_SYS visible to IRQ mode */
    SUB     sp,sp,#(2*4)        /* make room for stacking (r0_SYS, r1_SYS) */

    MSR     cpsr_c,#((CPSR_IRQ_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)) /* IRQ mode, no IRQ/FIQ */
    STMFD   r0!,{r13,r14}       /* finish saving the context (r0_SYS,r1_SYS)*/

    MSR     cpsr_c,#((CPSR_SYS_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)) /* SYS mode, no IRQ/FIQ */

    LDR     r12,=irq_handler
    MOV     lr,pc               /* copy the return address to link register */
    BX      r12                 /* call the C IRQ-handler (ARM/THUMB) */

    MSR     cpsr_c,#((CPSR_SYS_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)) /* SYSTEM mode, IRQ/FIQ disabled */
    MOV     r0,sp               /* make sp_SYS visible to IRQ mode */
    ADD     sp,sp,#(8*4)        /* fake unstacking 8 registers from sp_SYS */

    MSR     cpsr_c,#((CPSR_IRQ_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)) /* IRQ mode, both IRQ/FIQ disabled */
    MOV     sp,r0               /* copy sp_SYS to sp_IRQ */
    LDR     r0,[sp,#(7*4)]      /* load the saved SPSR from the stack */
    MSR     spsr_cxsf,r0        /* copy it into spsr_IRQ */

    LDMFD   sp,{r0-r3,r12,lr}^  /* unstack all saved USER/SYSTEM registers */
    NOP                         /* can''t access banked reg immediately */
    LDR     lr,[sp,#(6*4)]      /* load return address from the SYS stack */
    MOVS    pc,lr               /* return restoring CPSR from SPSR */

