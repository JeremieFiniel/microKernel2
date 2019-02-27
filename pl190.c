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
#include "pl190.h"

/*
 * The PL190 is a Virtual Interrupt Controller.
 * It is available on the Versatile Platform Board, among others.
 *
 */

struct pl190      *vic;
struct pl190_vectaddr  *vic_vectaddrs;
struct pl190_vectcntls *vic_vectcntls;

void vic_init(void) {
  vic = (struct pl190*)PL190_BAR0;
  vic_vectaddrs = (struct pl190_vectaddr*)PL190_BAR1;
  vic_vectcntls =  (struct pl190_vectcntls*)PL190_BAR2;
}


void vic_enable_irq(uint32_t irqno, uint32_t isr) {

  vic_vectaddrs->isrs[0] = isr;
  vic_vectcntls->srcs[0] = (1<<5) | irqno;
  vic->intr_select &= ~(1<<irqno);
  vic->intr_enable = 1<<irqno;
}

/*
 * One must read the PICVectAddr register, that tells the PIC that the interrupt is being serviced,
 * and the PIC priority handling is setup so that only higher priority interrupts are allowed.
 */
uint32_t vic_isr() {
  uint32_t isr = vic->vectaddr;
  return isr;
}

void vic_ack() {
  vic->vectaddr = 0;  // Acknowledge at the VIC level.
}


/*
 * Local Variables:
 * mode: c
 * tab-width: 2
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

