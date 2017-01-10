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
 */
	.global _arm_halt
	.func _arm_halt
_arm_halt:
    // disable IRQs and FIQs
    mrs r0, cpsr
    orr r0, r0, #(CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)
    msr cpsr_c, r0
	wfi
	b	_arm_halt
	.size  _arm_halt, . - _arm_halt
	.endfunc

	.global _arm_sleep
	.func _arm_sleep
_arm_sleep:
	wfi
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
 * NOTA BENE: this code works for an ARMv7, such as Cortex-A9.
 */

	.global _arm_irq_init
	.func _arm_irq_init
_arm_irq_init:

	mov r0, lr  @ save the return address in a non-banked register.

	/*----------------------------------------------
     * Initialize stack pointers for all ARM modes
     * The ARM processor has one user mode (USR) and 6 priviledged modes.
     * 5 of the 6 priviledged modes are exception modes:
     *    UND, ABT, SVC, FIQ, and IRQ
     * In exception modes, the ARM processor has banked r13 and r14,
     * that is, each exception mode has its own stack and link register.
     * Be carefull, the SYS and USR modes share the same stack and link register.
     *
     * Look in the ldscript, for the VExpress-A9 card, we have several stacks:
     *   One 4KB stack for the USR and SYS modes
     *   One 256B stack for the IRQ mode.
     *   One 256B stack for the SVC mode.
     *   One 256B stack shared by all other modes that are not used.
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

   /*
    * Reset the exception vector
    * Normally, we would copy the _primary and _secondary
    * at address 0x0000-0000, where the interrupt vector lies.
    * However, on the vexpress_a9 board, emulated by QEMU,
    * we haven't found a way to unmap the non-volatile memory
    * mapped at 0x0000-0000 when powering up the ARM board.
    *
    * Since the Cortex-A9 allows to have the interrupt vector
    * at another address than 0x0000-0000, we use this instead.
    *
   	* p15,0,<Rd>,c12,c0 -> Interrupt Vector base address
    * Cortex-A9 Technical Reference Manual
    * 4.2.13 c12 registers, page 59
	* Table 4-13 shows the CP15 system control registers you can access when CRn is c12.
    */
	mov r1,#0
	ldr r1,=_primary
	mcr p15, 0, r1, c12, c0, 0
	mrc p15, 0, r2, c12, c0, 0

	mrc p15, 0, r0, c1, c0, 0 @ fetch system control register (SCTLR)
	bic r1, r1, #(1<<13)      @ Force low interrupt vector
	mcr p15, 0, r0, c1, c0, 0 @ store system control register (SCTLR)

	/*
	 * Disable MMU
	 */
	mrc p15, 0, r1, c1, c0, 0 @ Read Control Register configuration data
	bic r1, r1, #0x1          @ Disable MMU
    mcr p15, 0, r1, c1, c0, 0 @ Write Control Register configuration data
	isb

	/*
	 * If no MMU, we can still enable L1 Caches (I-Cache and D-Cache),
	 * write buffers, barriers, and branch prediction.
	 */
	mrc p15, 0, r1, c1, c0, 0 @ Read Control Register configuration data
	orr r1, r1, #(0x1 << 12)  @ enable I Cache
	orr r1, r1, #(0x1 << 2)   @ enable D Cache
	orr r1, r1, #(0x1 << 3)   @ enable Write buffer
	orr r1, r1, #(0x1 << 5)   @ enable Barriers
	orr r1, r1, #(0x1 << 11)  @ branch prediction
	mcr p15, 0, r1, c1, c0, 0 @ Write Control Register configuration data


	//	data memory barrier
	mcr p15, 0, r0, c7, c10, 5

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
	.word _reset_loop
	.word _undef_loop
	.word _swi_handler  /* _softirq_loop */
	.word _arm_prefetch_abort
	.word _arm_data_abort
	.word _reserved_loop
	.word _arm_irq_handler
	.word _fiq_loop


/**
 * Switch to user mode, with the entry point given as argument (r0).
 * The idea is to save the return address on the stack (the SYS mode stack),
 * switch to USR mode, and then call the given entry point.

 * When the user code calls the syscall #0, it means that the current
 * user-level computation wants to terminate (in the sense of a process exit).
 * We get back the return address that we saved from the stack
 * (SYS mode stack, which the same as the USR mode stack,
 * since r13 and r14 registes are not banked between SYS and USR modes).
 *
 * We need to make sure that we respect the ABI. We chose the EABI compiler,
 * so we must respect the EABI calling convention, which is in fact ARM's AAPCS.
 * The complete EABI definitions currently live on ARM's infocenter:
 *
 *     http://infocenter.arm.com/help/topic/com.arm.doc.subset.swdev.abi/index.html
 *
 * The core points are the following:
 *
 * r0-r3 are the argument and scratch registers; r0-r1 are also the result registers
 * r4-r8 are callee-save registers
 * r9 might be a callee-save register or not (on some variants of AAPCS it is a special register)
 * r10-r11 are callee-save registers
 * r12-r15 are special registers
 *
 * A callee-save register must be saved by the callee (in opposition to a caller-save register,
 * where the caller saves the register); so, if this is the ABI you are using,
 * you do not have to save r10 before calling another function (the other function is responsible for saving it).
 *
 */

 _sp_save:
	 .word 0x00000000

	.global _arm_usr_mode
	.func _arm_usr_mode
	/* r0 = userno r1=entry_point r2=sp*/
_arm_usr_mode:
	push {r4-r11, lr}  @ push callee-saved registers.
	ldr r3, =_sp_save  @ save the SP register
	str sp, [r3]       @ somewhat equivalent to a setjmp() in C
	

	/* Switch from SYS_MODE to USR_MODE */
	mrs r3, cpsr
	bic r3,r3, #(CPSR_SYS_MODE | CPSR_IRQ_FLAG)
	orr r3,r3, #(CPSR_USR_MODE | CPSR_FIQ_FLAG)
	msr cpsr,r3
	/* Jump to the given entry point (r1), passing the userno (r0)
	 * so this function never returns to its caller,
	 * until the called entry point calls the syscall #0
	 * See below in the _swi_handler
	 */

	mov sp, r2

	mov pc, r1
	.size   _arm_usr_mode, . - _arm_usr_mode
	.endfunc

	.global _swi_exit
	.func _swi_exit
/*
 * This is the termination of a user-mode process.
 * Change to SYS mode, with IRQs and FIQs still disabled.
 */
 _swi_exit:

	cpsid if, #CPSR_SYS_MODE   @ Change to SYS mode, with IRQs and FIQs still disabled.

	ldr r0, =_sp_save          @ reload the SP register
	ldr sp, [r0]               @ somewhat equivalent to a longjmp() in C

	pop {r4-r11,lr}            @ popped callee-saved registers
	mov pc, lr                 @ return from _arm_usr_mode(void*)
	.size _swi_exit, . - _swi_exit
	.endfunc


/*
 * SWI instruction is composed from the instruction itself and the number given
 * when assembling it. So for instance:
 *
 *      swi 0x44
 *
 * Would produce the instruction: 0xef000044
 *
 * It is important to say that the swi forces a change in the processor mode,
 * from its current mode to the service mode (SVC). This requires that a stack
 * had been setup prior to executing swi instructions.
 *
 * It is also important to point out that the ARM processor handles software interrupts
 * differently than hardware interrupts. The ARM processor calls the SWI handler
 * with the LR register set on the next instruction to execute, right after
 * the swi instruction that induced the call to _swi_handler. So the LR register
 * is therefore correct. With hardware interrupts, the LR register would be wrong
 * and would need to be corrected.
 */

_swi_handler:

	/*
	 * Do not correct the link register, the ARM processor calls the SWI handler
	 * with the LR register set on the next instruction to execute, right after
	 * the swi instruction that induced the call to _swi_handler.
	 */
	ldr r3, [lr, #-4]  @ load the swi instruction
    and r3,r3,#0xFFFFFF
/*
 * now exit is in c code
 *	cmp     r3,#0 @exit
 *	beq     _swi_exit
 */

	/*
	 * Store the return state on the SYS mode stack.
	 * This includes SPSR (IRQ bank), which is the CPSR from SYS mode
	 * before we were interrupted, and link register (IRQ bank),
	 * which is the address to which we must return to continue
	 * the execution of the interrupted thread.
	 */
	srsdb #CPSR_SVC_MODE! /* srsdb: Store Return State Decrement Before */

	/* save register of spsr into svc stack */
	stmdb sp!, {r0-r12, sp, lr}^

	/* store the current ttrb */
	mrc p15, 0, r2, c2, c0, 0 /*get ttrb*/
	push {r2}
	mov r2, sp

	/*
	 * Change to SYS mode, with IRQs and FIQs still disabled.
	 */
	cpsid if, #CPSR_SYS_MODE /* Change Program State Interrupt Disable */

	/*
	 * Save on the SYS mode stack all callee-saved registers and LR register.
	 */
	push {r4-r11, lr}

	/* According to the document "Procedure Call Standard for the ARM
	 * Architecture", the stack pointer is 4-byte aligned at all times, but
	 * it must be 8-byte aligned when calling an externally visible
	 * function.  This is important because this code is reached from an IRQ
	 * and therefore the stack currently may only be 4-byte aligned.  If
	 * this is the case, the stack must be padded to an 8-byte boundary
	 * before up-calling the IRQ handler.
	 */
	and r4, sp, #4
	sub sp, sp, r4

	/* Call the board-level function that handles Interrupt ReQuests (IRQ). */
	bl swi_handler

	/*
	 * Restore the original stack alignment
	 * (see note about 8-byte alignment above).
	 */
	add sp, sp, r4

	/*
	 * Restore the above-mentioned registers from the SYS mode stack.
	 */
	pop {r4-r11, lr}

	/* go back to svc mode to set the old or new(depend of scheduling) context 
	 * need to be in svc mode to store all register in the spsr mode.
	 * sys mode don't have spsr.
	 */
	cpsid if, #CPSR_SVC_MODE /* Change Program State Interrupt Disable */
	/* get the ttbr */
	pop {r0}

	mcr p15, 0, r0, c2, c0, 0 /*set ttbr*/

	// Invalidate TLB
	mov r0, #0
	mcr p15, 0, r0, c8, c7, 0
	// Invalidate entire instruction TLB
	mcr p15, 0, r0, c8, c5, 0
	// Invalidate entire data TLB
	mcr p15, 0, r0, c8, c6, 0

	// set new spsr
	ldr r0, [sp, #64]
	msr spsr, r0

	// store register context on the spsr mode
	ldmia sp!, {r0-r12, sp, lr}^

	/*
	 * Load the original SYS-mode CPSR and PC that were saved on
	 * the SYS mode stack.
	 */
	rfeia sp! // rfeia: Return From Exception Increment After


_reset_loop: /* for debug, because we loop here, so we know why */
	b _reset_loop;

_undef_loop: /* for debug, because we loop here, so we know why */
	b _undef_loop;

_softirq_loop: /* for debug, because we loop here, so we know why */
	b _softirq_loop

_prefetch_loop: /* for debug, because we loop here, so we know why */
	b _prefetch_loop

_reserved_loop: /* for debug, because we loop here, so we know why */
	b _reserved_loop

_fiq_loop: /* for debug, because we loop here, so we know why */
	b _fiq_loop


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

_arm_prefetch_abort:
	/*
	 * Let's get at the reasons of the prefetch abort
	 * IFSR: Instruction Fault Status Register
	 * IFAR: Instruction Fault Address Register
	 *
	 */
	mrc p15, 0, r0, c5, c0, 1 @ Read IFSR
	mrc p15, 0, r1, c6, c0, 2 @ Read IFAR

    /*
     * Switch back to sys mode so that we have access
     * to the C stack from the debugger.
     */
	msr cpsr_c,#(CPSR_SYS_MODE | CPSR_IRQ_FLAG | CPSR_FIQ_FLAG)

	/* To help debugging, let's enter a forever loop */
1:
	b 1b

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

	/* To help debugging, let's enter a forever loop */
1:
	b 1b

_arm_irq_handler:

	/*
	 * First, correct the link register, because of how the ARM processor
	 * calls the IRQ handler. See ARM Architecture Reference Manual
	 * Section A2.6 page 54.
	 * Essentially, the LR register points to the next instruction,
	 * not the instruction that should be executed next when the interrupt
	 * handler returns.
	 */
	sub lr, lr, #4

	/*
	 * Store the return state on the SYS mode stack.
	 * This includes SPSR (IRQ bank), which is the CPSR from SYS mode
	 * before we were interrupted, and link register (IRQ bank),
	 * which is the address to which we must return to continue
	 * the execution of the interrupted thread.
	 */
	srsdb #CPSR_IRQ_MODE! /* srsdb: Store Return State Decrement Before */

	/* save register of spsr into irq stack */
	stmdb sp!, {r0-r12, sp, lr}^

	/* store the current ttbr */
	mrc p15, 0, r0, c2, c0, 0 /*get ttbr*/
	push {r0}
	mov r0, sp

	/*
	 * Change to SYS mode, with IRQs and FIQs still disabled.
	 */
	cpsid if, #CPSR_SYS_MODE /* Change Program State Interrupt Disable */

	/*
	 * Save on the SYS mode stack any registers that may be clobbered,
	 * namely the SYS mode LR and all other caller-save general purpose
	 * registers.  Also save r4 so we can use it to store the amount we
	 * decremented the stack pointer by to align it to an 8-byte boundary
	 * (see comment below).
	 */
	push {r0-r4, r12, lr}


	/* According to the document "Procedure Call Standard for the ARM
	 * Architecture", the stack pointer is 4-byte aligned at all times, but
	 * it must be 8-byte aligned when calling an externally visible
	 * function.  This is important because this code is reached from an IRQ
	 * and therefore the stack currently may only be 4-byte aligned.  If
	 * this is the case, the stack must be padded to an 8-byte boundary
	 * before up-calling the IRQ handler.
	 */
	and r4, sp, #4
	sub sp, sp, r4


	/* Call the board-level function that handles Interrupt ReQuests (IRQ). */
	bl irq_handler

	/*
	 * Restore the original stack alignment
	 * (see note about 8-byte alignment above).
	 */
	add sp, sp, r4

	/*
	 * Restore the above-mentioned registers from the SYS mode stack.
	 */
	pop {r0-r4, r12, lr}

	/* go back to irq mode to set the old or new(depend of scheduling) context 
	 * need to be in irq mode to store all register in the spsr mode.
	 * sys mode don't have spsr.
	 */
	cpsid if, #CPSR_IRQ_MODE /* Change Program State Interrupt Disable */
	/* get the ttbr */
	pop {r0}

	mcr p15, 0, r0, c2, c0, 0 /*set ttbr*/

	// Invalidate TLB
	mov r0, #0
	mcr p15, 0, r0, c8, c7, 0
	// Invalidate entire instruction TLB
	mcr p15, 0, r0, c8, c5, 0
	// Invalidate entire data TLB
	mcr p15, 0, r0, c8, c6, 0

	// set new spsr
	ldr r0, [sp, #64]
	msr spsr, r0

	// store register context on the spsr mode
	ldmia sp!, {r0-r12, sp, lr}^

	/*
	 * Load the original SYS-mode CPSR and PC that were saved on
	 * the SYS mode stack.
	 */
	rfeia sp! // rfeia: Return From Exception Increment After


