#ifndef HANDLER_H_
#define HANDLER_H_

#include "board.h"
#ifdef vexpress_a9
#include "gic.h"
#else
#include "pl190.h"
#endif

#define EVENT_LIST_SIZE 32

//need kmalloc so call it after heap initialize
void init_uart_device_driver();

void top_uart();
void bottom_uart();

void top_timer();

//void printfRoutine();

/*
 * structure for link of a linked list of bottom event
 */
struct Bottom_event{
	void (*bottom_func)(void);
	struct Bottom_event *next;
};

/*
 * strucutre for the linked list of bottom event
 * When a top is exected, he take a structure allocate in the free_event list,
 * fill it, and added at the tail of the pending event.
 * When it can, the kernel check if an event is in the head, 
 * and execute it if it is the case
 */
struct Bottom_event_list{
	struct Bottom_event* head;
	struct Bottom_event* tail;
	struct Bottom_event* free_event;
};

void init_bottom_event_list();
void free_bottom_event_list();
// add a bottom event on the pending list
void create_bottom_event(void (*func)(void));
// return the first pending bottom event, or null if the list is empty
void (*pending_bottom_event())(void);

#endif
