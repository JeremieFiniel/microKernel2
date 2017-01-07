#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#include "board.h"

/*
 * Context:
 *
 * 0: ttbr
 * 1: r0
 * .
 * .
 * .
 * 13: r12
 * 14: sp
 * 15: lr
 *
 * 16: pc
 * 17: cpsr
 *
 */
#define CONTEXT_SIZE 18

struct process{
	unsigned int pid; //process id
	uintptr_t ptp; //page table page
	uint32_t context[CONTEXT_SIZE]; //registers TODO
};

// link of process for linked list
struct process_link{ 
	struct process proc;
	struct process_link *next;
};

// use linked list with knowing first and last link for use FIFO
struct process_list{
	struct process_link *first;
	struct process_link *last;
};

struct process_list runing_processes;
struct process_list stop_processes;

struct process_link* current_process;

void scheduling_init();
//void create_first_process();
void create_user_process(void* entry);

void schedul(uint32_t* sp);

void exit_current_process(uint32_t *sp);

#endif
