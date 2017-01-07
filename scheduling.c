#include "scheduling.h"
#include "mmu.h"
#include "kmem.h"
#include "gic.h"

//#define SCHEDUL_VERBOSE

extern void umain(uint32_t pid);
extern uintptr_t _sys_stack_top;
extern void _arm_sleep(void);
unsigned int lastPID = 0;

void scheduling_init()
{
	lastPID = 0;
	current_process = NULL;
	runing_processes.first = NULL;
	runing_processes.last = NULL;

	stop_processes.first = NULL;
	stop_processes.last = NULL;
}

/*
void create_first_process()
{
	assert(current_process == NULL, "ERROR, first process called two times");
	struct process_link* process = kmalloc(sizeof(struct process_link));
	assert(process != NULL, "ERROR, not enough space in kernel heap to allocate new process");

	// TODO
	// WARNING, take care of concurrency here!!!
	process->proc.pid = lastPID;
	lastPID ++;

	process->proc.ptp = start_first_mmu();

	process->next = NULL;
	current_process = process;

	umain(process->proc.pid);
}
*/

void create_user_process(void* entry)
{
	// WARNING, take care of concurrency here!!!
	static uint32_t entry_ptr;
	entry_ptr = (uint32_t) entry;
	static int mmu_was_enable;
	mmu_was_enable	= get_mmu_enable();
	if (mmu_was_enable)
		disable_mmu();

	int i;

	struct process_link* process = kmalloc(sizeof(struct process_link));
	assert(process != NULL, "ERROR, not enough space in kernel heap to allocate new process");

	process->proc.pid = lastPID;
	lastPID ++;

	uintptr_t ptp = new_user_ptp();
	process->proc.ptp = ptp;
	process->next = NULL;

	for (i = 0; i < CONTEXT_SIZE; i ++)
		process->proc.context[i] = 0;
	process->proc.context[0] = ptp;
	process->proc.context[1] = process->proc.pid;
	process->proc.context[14] = (uint32_t)&_sys_stack_top;
	process->proc.context[16] = entry_ptr;


	uint32_t cpsr;

	__asm__ __volatile__(
			"mrs %0, cpsr\n"
			"bic %0,%0, #(0x1F|0x80)\n"
			"orr %0,%0, #(0x10|0x40)\n"
			: "=r" (cpsr)
			:
			: "memory");

	process->proc.context[17] = cpsr;
	// TODO need to set the sp reg

	if (runing_processes.last == NULL)
	{
		runing_processes.first = process; 
		runing_processes.last = process; 
	}
	else
	{
		runing_processes.last->next = process;
		runing_processes.last = process;
	}

	if (mmu_was_enable)
		enable_mmu();
}

void schedul(uint32_t* sp)
{
	int i;


	if (runing_processes.first != NULL)
	{
		for (i = 0; i < CONTEXT_SIZE; i ++) 
			current_process->proc.context[i] = sp[i];

#ifdef SCHEDUL_VERBOSE
		kprintf("Schedul from\n");
		kprintf("pid: 0x%x\n", current_process->proc.pid);
		kprintf("ttbr: 0x%x\n", sp[0]);
		kprintf("sp: 0x%x\n", sp[14]);
		kprintf("pc: 0x%x\n", sp[16]);
		kprintf("cpsr: 0x%x\n", sp[17]);
#endif

		struct process_link* process = runing_processes.first;
		runing_processes.first = process->next;
		if (runing_processes.first == NULL)
			runing_processes.last = NULL;

		if (runing_processes.last == NULL)
		{
			runing_processes.last = current_process;
			runing_processes.first = current_process;
		}
		else
		{
			runing_processes.last->next = current_process;
			runing_processes.last = current_process;
		}

		current_process = process;
		current_process->next = NULL;

		for (i = 0; i < CONTEXT_SIZE; i ++) //r0-r12, sp, lr
			sp[i] = current_process->proc.context[i];
#ifdef SCHEDUL_VERBOSE
		kprintf("Schedul to\n");
		kprintf("pid: 0x%x\n", current_process->proc.pid);
		kprintf("ttbr: 0x%x\n", sp[0]);
		kprintf("sp: 0x%x\n", sp[14]);
		kprintf("pc: 0x%x\n", sp[16]);
		kprintf("cpsr: 0x%x\n", sp[17]);
#endif
		//set_TTBR0(current_process->proc.ptp);
		//invalidate_tlb();
	}
}

void _swi_exit(void);

void exit_current_process(uint32_t* sp)
{
	int i = 0;
	//TODO
	//need to garbage the space use for the ptp and on the user space
	//Not done yet because not enough time
	
	kfree(current_process);
	current_process == NULL;

	if (runing_processes.first == NULL) // all processes are finished
	{
		kprintf("No more process in runging mode\n");
		/* TODO
		 * need to do something more clear here
		 * normaly never go here because umain have inifinit loop before call exit
		 */
		//disable_mmu();
		//_swi_exit();
		for (;;)
			_arm_sleep();
	}
	else
	{
		current_process = runing_processes.first;
		runing_processes.first = current_process->next;
		if (runing_processes.first == NULL)
			runing_processes.last = NULL;
		current_process->next == NULL;
		for (i = 0; i < CONTEXT_SIZE; i ++) //r0-r12, sp, lr
			sp[i] = current_process->proc.context[i];
#ifdef SCHEDUL_VERBOSE
		kprintf("Schedul\n");
		kprintf("pid: 0x%x\n", current_process->proc.pid);
		kprintf("ttbr: 0x%x\n", sp[0]);
		kprintf("sp: 0x%x\n", sp[14]);
		kprintf("pc: 0x%x\n", sp[16]);
		kprintf("cpsr: 0x%x\n", sp[17]);
#endif
	}
}
