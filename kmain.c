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
#include "pl011.h"
#ifdef vexpress_a9
#include "gic.h"
#include "gid.h"
#else
#include "pl190.h"
#endif
#include "kmem.h"
#include "handler.h"
#include "timer.h"
#include "mmu.h"
#include "scheduling.h"
#include "printf.h"

#define ECHO
#define ECHO_ZZZ
#undef ECHO_IRQ

extern void _arm_sleep(void);

struct pl011_uart* stdin;
struct pl011_uart* stdout;

/**
 * We do support two boards, the VExpress-A9 board and the VersatilePB board.
 * They have two different interrupt controllers, so this impacts the interrupt
 * handler and the interrupt controller setup.
 * This is the interrupt handler for the VExpress-A9.
 * On more recent ARM boards, the board usually relies on the standard GIC/GID
 * specification, created by ARM, of a generic interrupt controller and distributor.
 * This is an improvement from earlier ARM boards that each could pick a different
 * interrupt controller. Most of the time, with the ARM GIC/GID hardware, a single
 * generic software framework is enough to work with interrupts.
 * Look at the following files:
 *    - gic.s, gic.c, and gic.h
 *    - gid.c and gid.h
 */
#ifdef vexpress_a9

/**
 * This is a simple initialization to get interrupts from the UART0 (stdin).
 * Enable interrupts requires multiple steps on the VExpress board:
 *    - Initialize the GID (Generic Interrupt Distributor)
 *    - Initialize the GIC (Generic Interrupt Controller)
 *    - Enable the RX interrupt (receive interrupt) on the UART0
 *    - Enable the UART0 RX interrupt on the GID/GIC
 *    - Enable interrupts at the Cortex-A9 level
 */
void irq_init() {
	cortex_a9_gid_init();
	// cortex_a9_gid_dump_state();
	uart_send_string(stdout, "GID initialized.\n\r");
	cortex_a9_gic_init();
	// cortex_a9_gic_dump_state();
	uart_send_string(stdout, "GIC initialized.\n\r");

	/*
	 * Enable the RX interrupt on the UART0, our standard input (stdin).
	 * We do not need to enable any other interrupts, but many others exist.
	 */
	uart_enable_irqs(stdin,UART_IMSC_RXIM);
	cortex_a9_gid_enable_irq(UART0_IRQ);

	init_bottom_event_list();
	timer_init();
	timer_set_timer(1000000);
	timer_set_auto_load();
	timer_enable_irq();

	cortex_a9_gid_enable_irq(TIMER_IRQ);
	timer_enable();
}

/**
 * This is the interrupt handler. With ARM, there is one generic handler
 * for all interrupts (that is IRQs in the ARM parlance, usually FIQs are
 * handled by a different handler. See assembly setup in gic.s.
 */
void irq_handler(void* sp) {

	irq_id_t irq = 0;
	cpu_id_t cpu = 0;

	/*
	 * One generic handler means that the first step is asking the GIC
	 * which interrupt is active, that is, which is the current interrupt
	 * we are handling here.
	 */
	cortex_a9_gic_get_current_irq(&irq, &cpu);
	/*
	 * The current interrupt can be a spurious interrupt. One of the cause
	 * of spurious interrupts is a device clearing the interrupt in between
	 * the time the GIC notified the processor about the interrupt and the
	 * time the handler actually gets called. By the time the handler enquires
	 * about the current interrupt, there is none to report, so the GIC reports
	 * a spurious interrupt.
	 */
	if (ARM_GIC_IAR_SPURIOUS(irq))
		return;

	if (irq == UART0_IRQ) {
		/*
		 * You must do the read here first, from the UART0, before doing any print
		 * on the same serial line... Normally, this should not be necessary!
		 * The reason is obscure, it is because of an unexplained GCC behavior.
		 * For some unknown reason, GCC generates reads of the UART.DR register when writing to it...
		 * which therefore reads the pending character out of the FIFO and looses it.
		 * Also, the read may lower the IRQ line, if the read drops the number of pending
		 * characters in the receive FIFO below the RX interrupt threshold.
		 */
		top_uart();
	}
	else if (irq == TIMER_IRQ)
		top_timer((uint32_t*)sp);

#ifdef ECHO_IRQ
	kprintf("\n\r------------------------------\n\r");
	kprintf("  irq=%d cpu=%d \n\r", irq, cpu);
	kprintf("------------------------------\n\r");
#endif
	cortex_a9_gic_acknowledge_irq(irq, cpu);
}
#endif

/**
 * We do support two boards, the VExpress-A9 board and the VersatilePB board.
 * They have two different interrupt controllers, so this impacts the interrupt
 * handler here.
 *
 * This is the initialization and interrupt handler for the VersatilePB board.
 * The interrupt controller is called a VIC (Virtual Interrupt Controller),
 * it is a PL190 controller, so look at the code in pl190.s pl190.c and pl190.h.
 * On earlier ARM processor, the interrupt controller was like any other controller
 * on the bus, it appears as controller and it is driven like any controller.
 * This meant that different boards could have different interrupt controllers,
 * even they had the same ARM processor.
 */
#ifdef versatilepb

/**
 * This initialization for enabling interrupts, with the UART0 RX interrupt turned on,
 * is simpler on the older ARMv5 processor of the VersatilePB board.
 * The steps to enable interrupts are the following:
 *    - Initialize the VIC (Virtual Interrupt Controller)
 *    - Enable the UART0 RX interrupt on the VIC
 *    - Enable the RX interrupt (receive interrupt) on the UART0
 *    - Enable interrupts at the ARM CPU interface level
 */
void irq_init() {
	vic_init();
	vic_enable_irq(PL190_UART0_INTR,0x0000BABE);
	uart_enable_irqs(stdin,UART_IMSC_RXIM);

	timer_init();
	timer_enable_irq();
	timer_set_timer(100000);
	timer_set_auto_load();

	vic_enable_irq(PL190_TIMER0_INTR,0x00000100);
	timer_enable();
}

/**
 * This is the generic interrupt handler.
 * With ARM, there is one generic handler for all interrupts
 * (that is IRQs in the ARM parlance, usually FIQs are handled by a different handler.
 * See assembly setup in PL190.s.
 */
void irq_handler() {
	uint32_t isr = vic_isr();
	if (isr==(uint32_t)0x0000BABE) {
		top_uart();
	}
	else if (isr==(uint32_t)0x00000100){
		top_timer();
	}
	vic_ack();
}

#endif

/**
 * This is called from kprintf, it is the hook to print a character out.
 * As you can see, we currently print out on the UART0.
 */
void kputchar(int c, void *arg) {
	uart_send(UART0, c);
}

/**
 * This is just a trick to show that your processor is spinning
 * like crazy in between your character strokes...
 * This function has no other functional purpose than that.
 */
#ifdef ECHO_ZZZ
void zzz(void) {
	static uint32_t count = 0;
	count++;
	if (count > 60000000) {
		kprintf("Zzzz...\n\r");
		count = 0;
	}
}
#else
#define zzz() 
#endif

#ifdef CONFIG_TEST_MALLOC
#define NCHUNKS 124
void* chunks[NCHUNKS];
size_t sizes[NCHUNKS];
size_t nchunks=0;
#endif

/**
 * Polling approach to listen on "stdin" and echo on "stdout"
 */
void poll() {
	for (;;) {
		unsigned char c;
		zzz();
		if (0 == uart_receive(stdin, &c))
			continue;
		if (c == 13) {
			uart_send(stdout, '\r');
			uart_send(stdout, '\n');
		} else {
			uart_send(stdout, c);
		}
#ifdef CONFIG_TEST_MALLOC
		if (nchunks>=NCHUNKS || c==13) {
			size_t nfreed = 0;
			kprintf("Free %d chunks: \n\r",nchunks);
			for (int i=0;i<nchunks;i+=2) {
				kprintf("  chunks[%d]: %d bytes @ 0x%x \n\r",i,sizes[i],chunks[i]);
				free(chunks[i]);
				nfreed++;
			}
			for (int i=1;i<nchunks;i+=2) {
				kprintf("  chunks[%d]: %d bytes @ 0x%x \n\r",i,sizes[i],chunks[i]);
				free(chunks[i]);
				nfreed++;
			}
			assert(nfreed==nchunks," FIXME ");
			space_valloc_cleanup();
			nchunks = 0;
		}
		size_t size = c;
		if (size>= MAX_HOLE_SIZE)
			size = MAX_HOLE_SIZE;
		sizes[nchunks] = size;
		chunks[nchunks++]= malloc(size);
#endif
	}
}


extern void umain(uint32_t userno, uint32_t sp);
extern void kfflush();

void /* __attribute__ ((interrupt ("SWI"))) */ swi_handler (uint32_t r0, uint32_t r1, uint32_t sp, uint32_t no) {
	kprintf("SWI no=%d, r0=0x%x r1=0x%x sp=0x%x  \n",no,r0,r1,sp);

	if (no == 0) //exit
	{
		kprintf("Process %d exit with code %d\n", current_process->proc.pid, r0);
		exit_current_process((uint32_t*)sp);
	}
	else if (no == 2) //create process
	{
		kprintf("Create user process\n");	
		create_user_process((void*)r0);
	}
	else if (no == 3) //sleep
	{
		_arm_sleep();
	}
	else if (no == 4) //fflush
	{
		kfflush();
	}
}

/**
 * This is the C entry point, upcalled from assembly.
 * See startup.s
 */
void kmain() {
	int i = 0;

	stdin = UART0;
	uart_init(stdin);
#ifdef LOCAL_ECHO
	stdout = UART0;
#else
	stdout = UART1;
	uart_init(stdout);
#endif

	space_valloc_init();

	init_uart_device_driver();
	uart_send_string(stdout, "\n\nHello world!\n\r");
	uart_send_string(stdin,"Please type here...\n\r");
#ifndef LOCAL_ECHO
	uart_send_string(stdout,"\n\nCharacters will appear here...\n\r");
#endif

	init_printf();

#ifdef CONFIG_POLLING
	poll();
#else
	irq_init();
	uart_send_string(stdout, "IRQs enabled\n\r");
#ifdef vexpress_a9
	mmu_init();
	scheduling_init();
	create_user_process(umain);
	current_process = runing_processes.first;
	runing_processes.first = NULL;
	runing_processes.last = NULL;
	set_TTBR0(current_process->proc.context[0]);
	arm_enable_interrupts();
	enable_mmu();
	umain(0, current_process->proc.context[14]);
#else
	arm_enable_interrupts();
#endif
	uart_send_string(stdout, "No more process running\n\r");

	for (;;)
		_arm_sleep();
#endif
}


/*
 * Local Variables:
 * mode: c
 * tab-width: 2
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

