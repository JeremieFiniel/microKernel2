#include "handler.h"
#include "pl011.h"
#include "kmem.h"
#include "timer.h"
#include "scheduling.h"

static struct Bottom_event_list bottom_event_list;

#define UART_BUFFER_SIZE 16

struct UartBuffer{
	char *buffer;
	int start;
	int end;
};

static struct UartBuffer uartBuffer;

extern struct pl011_uart* stdin;
extern struct pl011_uart* stdout;

void init_uart_device_driver()
{
	uartBuffer.buffer = kmalloc(UART_BUFFER_SIZE);
	uartBuffer.start = 0;
	uartBuffer.end = 0;
}

void top_uart()
{
	char c;
	uart_receive(stdin, &c);
	if (((uartBuffer.end + 1) % UART_BUFFER_SIZE) != uartBuffer.start) 
	{
		uartBuffer.buffer[uartBuffer.end] = c;
		uartBuffer.end = (uartBuffer.end + 1) % UART_BUFFER_SIZE;

		create_bottom_event(&bottom_uart);

	}

	//acknowledge the irq;
	uart_ack_irqs(stdin);
}

void bottom_uart()
{
	char c;

	/*
	 * Here we do not have problem of concurrency becose top modify end variable and bottom modify start variable. 
	 * What will be possible is to just consume one character but it means charachter will take more time to be consume.
	 */

	while (uartBuffer.start != uartBuffer.end)
	{
		c = uartBuffer.buffer[uartBuffer.start];
		uartBuffer.start = (uartBuffer.start+1)%UART_BUFFER_SIZE; 
		if (c == 13) {
			uart_send(stdout, '\r');
			uart_send(stdout, '\n');
		} else {
			uart_send(stdout, c);
		}
	}
}

void top_timer(uint32_t* sp)
{
	timer_clear_irq();

	schedul(sp);


	//printfRoutine();
	void (*bottom_func)(void) = pending_bottom_event();
	while (bottom_func != NULL)
	{
		arm_enable_interrupts();
		bottom_func();
		arm_disable_interrupts();
		bottom_func = pending_bottom_event();
	}
}

void init_bottom_event_list()
{
	struct Bottom_event* event;
	event = kmalloc(sizeof(struct Bottom_event));
	bottom_event_list.free_event = event;
	for (int i = 0; i < EVENT_LIST_SIZE - 1; i ++)
	{
		event->next = kmalloc(sizeof(struct Bottom_event));
		event = event->next;
	}
	event->next = NULL;
	bottom_event_list.head = NULL;
	bottom_event_list.tail = NULL;

}

void free_bottom_event_list()
{
	// TODO
}

// add a bottom event on the pending list
void create_bottom_event(void (*func)(void))
{
	struct Bottom_event* event;
	event = bottom_event_list.free_event;
	if (event != NULL)
	{
		event->bottom_func = &bottom_uart; // fill the event
		bottom_event_list.free_event = event->next; // remove from the free list
		if (bottom_event_list.head == NULL) // list is empty
		{
			bottom_event_list.head = event;
			bottom_event_list.tail = event;
		}
		else //add the event on the tail
			bottom_event_list.tail->next = event;
		event->next = NULL;
	}
}

// return the first pending bottom event, or null if the list is empty
void (*pending_bottom_event())(void)
{
	struct Bottom_event* event;
	event = bottom_event_list.head;
	if (event == NULL) // no pending event
		return NULL;

	void (*func)(void) = event->bottom_func;

	struct Bottom_event* tmpEvent;

	tmpEvent = event->next;
	event->next = bottom_event_list.free_event;
	bottom_event_list.free_event = event;
	bottom_event_list.head = tmpEvent;
	if (bottom_event_list.head == NULL) // list is empty
		bottom_event_list.tail = NULL;

	return func;
}

/*
   extern char* printfBuf;
   extern int printfStart;
   extern int printfEnd;


   void printfRoutine()
   {
   while (printfStart != printfEnd)
   {
   uart_send(UART0, printfBuf[printfEnd]);
   printfEnd = (printfEnd + 1)%PRINTFBUFSIZE;
   }
   }
   */
