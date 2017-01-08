/*
 * user.c
 *
 *  Created on: Dec 2, 2016
 *      Author: ogruber
 */


#include <stddef.h>
#include <stdint.h>
#include "printf.h"

static int errcode = 0;
//void kprintf(const char *fmt, ...);
void _arm_usr_mode(uint32_t userno, void* entry, uint32_t sp);
void _arm_halt(void);

extern void _arm_sleep(void);

void exit(int code) {
	errcode = code;
	__asm volatile ("swi #0x00" ::: );
	_arm_halt(); // should never execute!
}

void sleep(uint32_t delay) {
	__asm volatile ("swi #0x01" ::: );
}

void create_user_process_syscall(void* entry) {
	__asm volatile ("swi #0x02" :::);
}

void prog1(uint32_t pid) {

	printf("USER[%d]: prog1!\n",pid);

	int i;
	for (i = 0; i < 15; i ++)
		printf("1Hello world!\n");

	exit(0);
}

void prog2(uint32_t pid) {

	printf("USER[%d]: prog2!\n",pid);

	int i;
	for (i = 0; i < 15; i ++)
		printf("2Hi you\n");

	exit(0);
}

/*
 * WARNING: do not change this signature without changing the assembly code
 * in gic.s -> see _arm_usr_mode
 */
void main(uint32_t pid) {
	printf("USER[%d]: Hello!\n",pid);
	/*
	 * Attempt to change to SYS mode, by changing the CPSR
	 * That can only be done in privileged mode.
	 * Notice that the code does not crash, but has no effect.
	 * So this is an example of a sensitive instruction that does not trap
	 */
	unsigned long old, niu;
	__asm__ __volatile__(
			"mrs %0, cpsr\n"
			"orr %1, %0, #0x1F \n"
			"msr cpsr, %1\n"
			"mrs %1, cpsr"
			: "=r" (old), "=r" (niu)
			:
			: "memory");
	if (old!=niu) {
		printf("USR mode succeeded to change to SYS mode");
		printf(" cpsr=0x%x -> cpsr=0x%x \n",old,niu);
	} else
		printf(" -- in USR mode!\n");

	create_user_process_syscall(prog1);
	create_user_process_syscall(prog2);
	create_user_process_syscall(prog1);

	/*
	 * Now try one syscall (not implemented, will do nothing.
	 */
	for (;;)
		_arm_sleep();

	/*
	 * Terminate this "process"...
	 */
	exit(0);
}

void umain(uint32_t pid, uint32_t sp) {
	printf("--> launching user pid=%d...\n",pid);
	//_arm_usr_mode(pid, main, sp);
	/* Switch from SYS_MODE to USR_MODE */
	__asm__ __volatile__(
			"mrs r0, cpsr\n"
			"bic r0,r0, #0x1f\n" //(CPSR_SYS_MODE | CPSR_IRQ_FLAG)
			"orr r0,r0, #0x50\n" //(CPSR_USR_MODE | CPSR_FIQ_FLAG)
			"msr cpsr,r0\n"

			"mov sp, %0\n"
			"mov pc, %1\n"
			::"r" (sp), "r" (main): "memory"
			);
	printf("--> user pid=%d exited, errcode=%d \n",pid, errcode);
}
