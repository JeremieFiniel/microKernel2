#ifndef TIMER_H_
#define TIMER_H_

#ifdef vexpress_a9
/*
 * Cortex_a9 MPCore, Thechnical Reference Manual
 * 4.2 Private timer and watchdog registers (page 64)
 */

#define ARM_TIMER_LR 0x0000		// load register
#define ARM_TIMER_COUR 0x0004	// counter register
#define ARM_TIMER_CR 0x0008 	// control register
#define ARM_TIMER_ISR 0x000C 	// interrupt state register

#define ARM_TIMER_CR_TE (1<<0)	// time enable
#define ARM_TIMER_CR_AR (1<<1)	// auto reload
#define ARM_TIMER_CR_IE (1<<2)	// irq enable
struct timer_cr{
	unsigned int enable :1;
	unsigned int auto_load :1;
	unsigned int irq_enable :1;
	unsigned int :5;
	unsigned int prescal :8;
	unsigned int :16;
};

#else

/*
 * DDIO271_timer.pdf
 * 3.1 Summary of registers (page 34)
 */

#define TIMER_BASE 0x101E2000
#define TIMER_LR 0x00	// load register
#define TIMER_CR 0x08	// control register
#define TIMER_ICR 0x0C	// clear interrupt

struct timer_cr{
	unsigned int one_shot :1;
	unsigned int size :1; // 16/32 bits
	unsigned int prescal :2; //
	unsigned int :1;
	unsigned int irq_enable :1;
	unsigned int mode :1;
	unsigned int enable :1;
	unsigned int :24;
};

#endif

void timer_init();
void timer_enable();
void timer_disable();

void timer_enable_irq();
void timer_disable_irq();
void timer_clear_irq();
void timer_set_timer(unsigned int t);
void timer_set_auto_load();
void timer_set_single_shot();
void timer_set_prescal(unsigned int prescal);

#endif
