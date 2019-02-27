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
 * This is the entry point, see the linker script
 */
    .text
     .section .startup,"x"
    .code 32
 	.global _entry
_entry:

	bl _arm_irq_init

	//-------------------------------------------
	// we're loaded at _load and we relocate to _start,
	// but only if they are different.
	//-------------------------------------------
.relocate:
	ldr     r3,=_load 
	ldr	r4, =_start
	cmp     r3,r4
	beq     .clear
	//-------------------------------------------
	// copy from r3 to r4, until r9
	// 16 bytes at a time (through {r5-58})
	//-------------------------------------------
	ldr	r9, =_bss_start 
1:
	ldmia	r3!, {r5-r8} // load multiple from r3
	stmia	r4!, {r5-r8} // store multiple at r4

	cmp	r4, r9 // are we done yet?
	blo	1b

	//-------------------------------------------
	// Clear out bss.
	// we clear bss from r4 to r9, 
	// 16 bytes at a time (through {r5-58})
	//-------------------------------------------
.clear:
	ldr	r4, =_bss_start
	ldr	r9, =_bss_end
	mov	r5, #0
1:
	stmia	r4!, {r5} 
	cmp	r4, r9
	blo	1b

	/*
	 * We can upcall the C entry point now.
	 * We have initialized the stacks for the different mode
	 * We have relocated ourself if it was necessary
	 * We have zero-ed all C variables that needed to be (bss)
	 */
    bl kmain

    /*
     * In the C funtion kmain() returns, just fall through to the halt code.
     */
	b _arm_halt
