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

#ifndef PL011_H_
#define PL011_H_

/**
 * PL011_T UART
 *     http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183f/DDI0183.pdf
 *
 * UARTDR: Data Register   (0x00)
 *    To read received bytes
 *    To write bytes to send
 *    Bit Fields:
 *      15:12 reserved
 *      11:08 error flags
 *       7:0  data bits
 * UARTFR:  Flag Register  (0x18)
 *    Bit Fields:
 *      7:  TXFE  transmit FIFO empty
 *      6:  RXFF  receive FIFO full
 *      5:  TXFF  transmit FIFO full
 *      4:  RXFE  receive FIFO empty
 *      3:  BUSY  set when the UART is busy transmitting data
 */

#define UART_DR 0x00
#define UART_FR 0x18

#define UART_TXFE (1<<7)
#define UART_RXFF (1<<6)
#define UART_TXFF (1<<5)
#define UART_RXFE (1<<4)
#define UART_BUSY (1<<3)

/**
 * Nota Bene:
 *   You must disable the UART before any of the control registers are reprogrammed.
 *   When the UART is disabled in the middle of transmission or reception,
 *   it completes the current character before stopping.
 *
 * Nota Bene:
 *   The received data character must be read first from UARTDR before reading
 *   the error status associated with that data character from UARTRSR.
 *   This read sequence cannot be reversed, because the status register UARTRSR is updated
 *   only when a read occurs from the data register UARTDR. However, the status information
 *   can also be obtained by reading the UARTDR register
 */

struct __attribute__ ((__packed__)) pl011_uart {
 volatile uint32_t DR;              /* 0x00   Data register */
 volatile uint32_t RSR_ECR;         /* 0x04   Receive status register/error clear register */
 volatile uint8_t reserved1[0x10];  /* 0x08-0x14 Reserved */
 volatile const uint32_t FR;        /* 0x18   Flag register */
 volatile uint8_t reserved2[0x4];   /* 0x1C   reserved */
 volatile uint32_t LPR;             /* 0x20   IrDA low-power counter register */
 volatile uint32_t IBRD;            /* 0x24   Integer baud rate register */
 volatile uint32_t FBRD;            /* 0x28   Fractional baud rate register */
 volatile uint32_t LCR_H;           /* 0x2C   Line control register */
 volatile uint32_t CR;              /* 0x30   Control register */
 volatile uint32_t IFLS;            /* 0x34   Interrupt FIFO level select register */
 volatile uint32_t IMSC;            /* 0x38   Interrupt mask set/clear register */
 volatile const uint32_t RIS;       /* 0x3C   Raw interrupt status register */
 volatile const uint32_t MIS;       /* 0x40   Masked interrupt status register */
 volatile uint32_t ICR;             /* 0x44   Interrupt clear register */
 volatile uint32_t DMACR;           /* 0x48   DMA control register */
};

/**
 * 15 CTSEn CTS hardware flow control enable. If this bit is set to 1,
 *          CTS hardware flow control is enabled. Data is only transmitted
 *          when the nUARTCTS signal is asserted.
 * 14 RTSEn RTS hardware flow control enable. If this bit is set to 1,
 *          RTS hardware flow control is enabled. Data
 *          is only requested when there is space in the receive FIFO for it to be received.
 * 13 Out2  This bit is the complement of the UART Out2 (nUARTOut2) modem status output.
 *          That is, when the bit is programmed to a 1, the output is 0. For DTE this can be used
 *          as Ring Indicator (RI).
 * 12 Out1  This bit is the complement of the UART Out1 (nUARTOut1) modem status output.
 *          That is, when the bit is programmed to a 1 the output is 0.
 *          For DTE this can be used as Data Carrier Detect (DCD).
 * 11 RTS   Request to send. This bit is the complement of the UART request to send (nUARTRTS) modem
 *          status output. That is, when the bit is programmed to a 1, the output is 0.
 * 10 DTR   Data transmit ready. This bit is the complement of the UART data transmit ready (nUARTDTR)
 *          modem status output. That is, when the bit is programmed to a 1, the output is 0.
 * 9  RXE   Receive enable. If this bit is set to 1, the receive section of the UART is enabled.
 *          Data reception occurs for either UART signals or SIR signals according to the setting
 *          of SIR Enable (bit 1). When the UART is disabled in the middle of reception,
 *          it completes the current character before stopping.
 * 8  TXE   Transmit enable. If this bit is set to 1, the transmit section of the UART is enabled.
 *          Data transmission occurs for either UART signals, or SIR signals according to
 *          the setting of SIR Enable (bit 1). When the UART is disabled in the middle of transmission,
 *          it completes the current character before stopping.
 * 7  LBE   Loop back enable. If this bit is set to 1 and the SIR Enable bit is set to 1 and
 *          the test register UARTTCR bit 2 (SIRTEST) is set to 1, then the nSIROUT path is inverted,
 *          and fed through to the SIRIN path. The SIRTEST bit in the test register must be set to 1
 *          to override the normal half-duplex SIR operation. This must be the requirement for accessing
 *          the test registers during normal operation, and SIRTEST must be cleared to 0 when loopback
 *          testing is finished.This feature reduces the amount of external coupling required during
 *          system test. If this bit is set to 1, and the SIRTEST bit is set to 0, the UARTTXD path
 *          is fed through to the UARTRXD path. In either SIR mode or normal mode, when this bit is set,
 *          the modem outputs are also fed through to the modem inputs.
 *          This bit is cleared to 0 on reset, which disables the loopback mode.
 * 6:3 - Reserved, do not modify, read as zero.
 * 2  SIRLP IrDA SIR low power mode.
 *          This bit selects the IrDA encoding mode. If this bit is cleared to 0,
 *          low-level bits are transmitted as an active high pulse with a width of 3/16th
 *          of the bit period. If this bit is set to 1, low-level bits are transmitted with
 *          a pulse width which is 3 times the period of the IrLPBaud16 input signal,
 *          regardless of the selected bit rate. Setting this bit uses less power, but
 *          might reduce transmission distances.
 * 1 SIREN  SIR enable. If this bit is set to 1, the IrDA SIR ENDEC is enabled.
 *          This bit has no effect if the UART is not enabled by bit 0 being set to 1.
 *          When the IrDA SIR ENDEC is enabled, data is transmitted and received on nSIROUT and SIRIN.
 *          UARTTXD remains in the marking state (set to 1). Signal transitions on UARTRXD or modem
 *          status inputs have no effect.
 *          When the IrDA SIR ENDEC is disabled, nSIROUT remains cleared to 0 (no light pulse generated),
 *          and signal transitions on SIRIN have no effect.
 * 0 UARTEN UART enable. If this bit is set to 1, the UART is enabled. Data transmission and reception
 *          occurs for either UART signals or SIR signals according to the setting of SIR Enable (bit 1).
 *          When the UART is disabled in the middle of transmission or reception, it completes
 *          the current character before stopping.
 */
#define UART_CR_UARTEN (1<<0)
#define UART_CR_TXE    (1<<8)
#define UART_CR_RXE    (1<<9)

/**
 * Interrupt Mask Set/Clear register
 * It is a read/write register.
 * On a read this register gives the current value of the mask on the relevant interrupt.
 * On a write of 1 to the particular bit, it sets the corresponding mask of that interrupt.
 * A write of 0 clears the corresponding mask.
 *
 * 15:11      Reserved, read as zero, do not modify.
 * 10         OEIM: Overrun error interrupt mask. On a read, the current mask for the OEIM interrupt is returned.
 *            On a write of 1, the mask of the OEIM interrupt is set. A write of 0 clears the mask.
 * 9          BEIM: Break error interrupt mask. On a read the current mask for the BEIM interrupt is returned.
 *            On a write of 1, the mask of the BEIM interrupt is set. A write of 0 clears the mask.
 * 8          PEIM:  Parity error interrupt mask. On a read the current mask for the PEIM interrupt is returned.
 *            On a write of 1, the mask of the PEIM interrupt is set. A write of 0 clears the mask.
 * 7          FEIM: Framing error interrupt mask. On a read the current mask for the FEIM interrupt is returned.
 *            On a write of 1, the mask of the FEIM interrupt is set. A write of 0 clears the mask.
 * 6          RTIM: Receive timeout interrupt mask. On a read the current mask for the RTIM interrupt is returned.
 *            On a write of 1, the mask of the RTIM interrupt is set. A write of 0 clears the mask.
 * 5          TXIM: Transmit interrupt mask. On a read the current mask for the TXIM interrupt is returned.
 *            On a write of 1, the mask of the TXIM interrupt is set. A write of 0 clears the mask.
 * 4          RXIM: Receive interrupt mask. On a read the current mask for the RXIM interrupt is returned.
 *            On a write of 1, the mask of the RXIM interrupt is set. A write of 0 clears the mask.
 * 3          DSRMIM: nUARTDSR modem interrupt mask. On a read the current mask for the DSRMIM interrupt is returned.
 *            On a write of 1, the mask of the DSRMIM interrupt is set. A write of 0 clears the mask.
 * 2          DCDMIM: nUARTDCD modem interrupt mask. On a read the current mask for the DCDMIM interrupt is returned.
 *            On a write of 1, the mask of the DCDMIM interrupt is set. A write of 0 clears the mask.
 * 1          CTSMIM: nUARTCTS modem interrupt mask. On a read the current mask for the CTSMIM interrupt is returned.
 *            On a write of 1, the mask of the CTSMIM interrupt is set. A write of 0 clears the mask.
 * 0          RIMIM: nUARTRI modem interrupt mask. On a read the current mask for the RIMIM interrupt is returned.
 *            On a write of 1, the mask of the RIMIM interrupt is set. A write of 0 clears the mask.
 */
#define UART_IMSC 0x038
#define UART_IMSC_OEIM (1<<10)
#define UART_IMSC_BEIM (1<<9)
#define UART_IMSC_PEIM (1<<8)
#define UART_IMSC_FEIM (1<<7)
#define UART_IMSC_RTIM (1<<6)
#define UART_IMSC_TXIM (1<<5)
#define UART_IMSC_RXIM (1<<4)
#define UART_IMSC_DSRMIN (1<<3)
#define UART_IMSC_DCDMIN (1<<2)
#define UART_IMSC_CTSMIN (1<<1)
#define UART_IMSC_RIMIN (1<<0)

/**
 * Interrupt Clear Register (write-only)
 * The UARTICR register is the interrupt clear register and is write-only.
 * On a write of 1, the corresponding interrupt is cleared.
 * A write of 0 has no effect.
 *
 * 15:11     Reserved, read as zero, do not modify
 * 10        OEIC:  Overrun error interrupt clear. Clears the UARTOEINTR interrupt.
 *  9        BEIC:  Break error interrupt clear. Clears the UARTBEINTR interrupt.
 *  8        PEIC:  Parity error interrupt clear. Clears the UARTPEINTR interrupt.
 *  7        FEIC:  Framing error  interrupt clear. Clears the UARTFEINTR interrupt.
 *  6        RTIC:  Receive timeout interrupt clear. Clears the UARTRTINTR interrupt.
 *  5        TXIC:  Transmit interrupt clear. Clears the UARTTXINTR interrupt.
 *  4        RXIC:  Receive interrupt clear. Clears the UARTRXINTR interrupt.
 *  3        DSRMIC: nUARTDSR modem interrupt clear. Clears the UARTDSRINTR interrup.
 *  2        DCDMIC: nUARTDCD modem interrupt clear. Clears the UARTDCDINTR interrupt.
 *  1        CTSMIC: nUARTCTS modem interrupt clear. Clears the UARTCTSINTR interrupt.
 *  0        RIMIC: nUARTRI modem interrupt clear. Clears the UARTRIINTR interrupt.
 */
#define UART_ICR 0x044
#define UART_ICR_OEIC (1<<10)
#define UART_ICR_BEIC (1<<9)
#define UART_ICR_PEIC (1<<8)
#define UART_ICR_FEIC (1<<7)
#define UART_ICR_RTIC (1<<6)
#define UART_ICR_TXIC (1<<5)
#define UART_ICR_RXIC (1<<4)
#define UART_ICR_DSRMIC (1<<3)
#define UART_ICR_DCDMIC (1<<2)
#define UART_ICR_CTSMIC (1<<1)
#define UART_ICR_RIMIC (1<<0)

extern void uart_init(struct pl011_uart* uart);


/**
 * Receive a byte from the given serial line.
 */
extern int uart_receive(struct pl011_uart* uart, unsigned char *s);

/**
 * Send a byte through the given serial line.
 */
extern void uart_send(struct pl011_uart* uart, const unsigned char c);

/**
 * Utility function to send a null-terminated string
 * of bytes through the given serial line.
 */
extern void uart_send_string(struct pl011_uart* uart, const unsigned char *s);

extern void uart_enable_irqs(struct pl011_uart* uart, uint32_t irqs);

extern void uart_disable_irqs(struct pl011_uart* uart, uint32_t irqs);

extern void uart_clear_irqs(struct pl011_uart* uart, uint32_t irqs);

extern void uart_clear_all_irqs(struct pl011_uart* uart);

extern void uart_ack_irqs(struct pl011_uart* uart);

#endif /* PL011_H_ */
