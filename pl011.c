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


#include <stddef.h>
#include <stdint.h>
#include "pl011.h"

/**
 * The PL011 is a UART controller.
 *
 * This is really a minimal initialization, assuming the first boot
 * code on the platform has already initialized the UART...
 * something that QEMU does.
 *
 * Normally, the UART should be reset and fully initialized,
 * to make sure we have 115200 bauds, no parity, 1 stop bit,
 * and no control flow. This is the usual setting for terminals.
 */

void uart_init(struct pl011_uart* uart) {

  uart->CR &= ~(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
  uart->ICR = 0x7FFF;
  uart->IMSC = 0x00; // &= ~(UART_IMSC_RXIM | UART_IMSC_TXIM);
  uart->CR |= (UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
}

/**
 * Receive a byte from the given serial line.
 */
int uart_receive(struct pl011_uart* uart, unsigned char *s) {
  if (uart->FR & UART_RXFE)
    return 0;
  *s = (uart->DR & 0xff);
  return 1;
}

/**
 * Send a byte through the given serial line.
 */
void uart_send(struct pl011_uart* uart, const unsigned char c) {
  while (uart->FR & UART_TXFF);
  uart->DR = c;
}

/**
 * Utility function to send a null-terminated string 
 * of bytes through the given serial line.
 */
void uart_send_string(struct pl011_uart* uart, const unsigned char *s) {
  while(*s != '\0') {
    uart_send(uart,*s);
    s++;
  }
}

/*
 * Enable the given interrupts on the given UART
 */
void uart_enable_irqs(struct pl011_uart* uart, uint32_t irqs) {
  uart->CR &= ~(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
  uart->ICR = 0x7FFF;
  uart->IMSC = irqs;
  uart->IFLS = 0x02;
  uart->CR |= (UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
}

/*
 * Disable the given interrupts on the given UART
 */
void uart_disable_irqs(struct pl011_uart* uart, uint32_t irqs) {
  uart->CR &= ~(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
  uart->ICR = 0x7FFF;
  uart->IMSC &= ~irqs;
  uart->IFLS = 0x02;
  uart->CR |= (UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
}

/*
 * Clear all interrupts, at the pl011 level.
 */
void uart_clear_all_irqs(struct pl011_uart* uart) {
  uart->ICR = 0x7FFF;
}

/*
 * Clear the given interrupts, at the pl011 level.
 */
void uart_clear_irqs(struct pl011_uart* uart, uint32_t irqs) {
  uart->ICR = irqs;
}

/*
 * Acknowledge the given interrupts, at the pl011 level.
 * This is a no-op for the PL011 since the interrupts are
 * self-acknowledged when reading from the DR register,
 * that is, when reading the available characters.
 *
 * Note: one could wonder if we should clear anything here,
 * or more specifically should we clear the RX interrupt?
 * Indeed, the RX interrupt is related to the amount of
 * received character in the FIFO, which is something
 * the software does not know about.
 * It is therefore not clear if the hardware will keep
 * the interrupt raised when cleared if the FIFO is
 * above threshold. Looking at the QEMU emulation of the PL011,
 * the interrupt will be lowered until the next receive.
 * But if there is no next receive, some characters will remain
 * in the FIFO...
 *
 * This seems to indicate that all received characters need
 * to be read by the IRQ handler... but there is still a race
 * condition between emptying the FIFO and receiving characters,
 * especially if the interrupt threshold is one character in the
 * FIFO.
 *
 * Note: a similar discussion about the TX interrupt is needed,
 *       although the RX and TX interrupts behave differently.
 *       The RX interrupt is high when the RX FIFO is above a
 *       threshold. The TX interrupt is high when the TX FIFO is below
 *       a threshold, so an empty FIFO would raise the TX interrupt.
 *       This means that if there is nothing to write out,
 *       the software must be able to clear the TX interrupt,
 *       avoiding to be "polled" by the device.
 */
void uart_ack_irqs(struct pl011_uart* uart) {
  uart->ICR = uart->MIS & ~UART_IMSC_RXIM;
}


/*
 * Local Variables:
 * mode: c
 * tab-width: 2
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

