#include "timer.h"
#include "board.h"


#ifdef vexpress_a9
static uint32_t periph_base = 0x0;
volatile static struct timer_cr* timer_cr = NULL;

void timer_init()
{
	periph_base = cortex_a9_peripheral_base();
	timer_cr = (struct timer_cr*)(periph_base + ARM_PWT_BASE_OFFSET + ARM_TIMER_CR);
}

void timer_enable()
{
	timer_cr->enable = 1;
}

void timer_disable()
{
	timer_cr->enable = 0;
}

void timer_enable_irq()
{
	timer_cr->irq_enable = 1;
}

void timer_disable_irq()
{
	timer_cr->irq_enable = 0;
}

void timer_clear_irq()
{
	// The event flag is cleared when written to 1.
	arm_mmio_write32(periph_base + ARM_PWT_BASE_OFFSET, ARM_TIMER_ISR, 1);
}

void timer_set_timer(unsigned int t)
{
	arm_mmio_write32(periph_base + ARM_PWT_BASE_OFFSET, ARM_TIMER_LR, t);
}

void timer_set_auto_load()
{
	timer_cr->auto_load = 1;
}

void timer_set_single_shot()
{
	timer_cr->auto_load = 0;
}

void timer_set_prescal(unsigned int prescal)
{
	assert(prescal < 0x10, "Error, prescal 0x%x endefined", prescal);
	timer_cr->prescal = prescal;
}

#else
volatile static struct timer_cr* timer_cr = NULL;

void timer_init()
{
	timer_cr = (struct timer_cr*)(TIMER_BASE + TIMER_CR);
	timer_cr->size = 1;
}
void timer_enable()
{
	timer_cr->enable = 1;
}

void timer_disable()
{
	timer_cr->enable = 0;
}

void timer_enable_irq()
{
	timer_cr->irq_enable = 1;
}

void timer_disable_irq()
{
	timer_cr->irq_enable = 0;
}

void timer_clear_irq()
{
	arm_mmio_write32(TIMER_BASE, TIMER_ICR, 0);
}

void timer_set_timer(unsigned int t)
{
	arm_mmio_write32(TIMER_BASE, TIMER_LR, t);
}

void timer_set_auto_load()
{
	timer_cr->mode = 0;
	timer_cr->one_shot = 0;

}

void timer_set_single_shot()
{
	timer_cr->one_shot = 1;
}

/*
 * DDIO271_timer.pdf
 * 3.2.3 Control Register, TimerXControl (page 38)
 *
 * Prescale bits:
 * 00 = 0 stages of prescale, clock is divided by 1 (default)
 * 01 = 4 stages of prescale, clock is divided by 16
 * 10 = 8 stages of prescale, clock is divided by 256
 * 11 = Undefined, do not use.
 */
void timer_set_prescal(unsigned int prescal)
{
	assert(prescal <= 0b10, "Error, prescal 0x%x endefined", prescal);
	timer_cr->prescal = prescal;
}

#endif
