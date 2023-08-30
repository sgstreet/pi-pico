/*
 * console.c
 *
 *  Created on: Feb 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#include <board/board.h>
#include <devices/console.h>

#include <sys/iob.h>
#include <sys/irq.h>

static void uart_isr(IRQn_Type irq, void *context)
{
	assert(context != 0);

	struct console *console = context;

	/* Save and clear the interrupt status register */
	uint32_t int_status = console->uart->UARTMIS;
	console->uart->UARTICR = int_status;

	/* Should we empty the RX FIFO? */
	while ((console->uart->UARTFR & UART0_UARTFR_RXFE_Msk) == 0) {
		uint32_t c = console->uart->UARTDR;
		if ((c & (UART0_UARTDR_OE_Msk | UART0_UARTDR_BE_Msk | UART0_UARTDR_PE_Msk | UART0_UARTDR_FE_Msk)) == 0) {
			osDequePutBack(console->rx_queue, &c, 0);
		} else {
			console->oe_counter += (c & UART0_UARTDR_OE_Msk) >> UART0_UARTDR_OE_Pos;
			console->be_counter += (c & UART0_UARTDR_BE_Msk) >> UART0_UARTDR_BE_Pos;
			console->pe_counter += (c & UART0_UARTDR_PE_Msk) >> UART0_UARTDR_PE_Pos;
			console->fe_counter += (c & UART0_UARTDR_FE_Msk) >> UART0_UARTDR_FE_Pos;
		}
	}

	/* Any thing to transmit? */
	while (osDequeGetCount(console->tx_queue)) {

		/* Room is the fifo? */
		if ((console->uart->UARTFR & UART0_UARTFR_TXFF_Msk) != 0)
			break;

		/* Add charater to the tx fifo */
		uint8_t c;
		osDequeGetFront(console->tx_queue, &c, 0);
		console->uart->UARTDR = c;
	}
}

int console_ini(struct console *console, UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size)
{
	assert(console != 0 && uart != 0);
	osDequeDelete(console->rx_queue);

	/* Basic initialization */
	memset(console, 0, sizeof(struct console));
	console->uart = uart;
	console->irq = irq;
	console->rx_timeout = osWaitForever;
	console->tx_timeout = osWaitForever;

	/* Initialize the rx queue */
	console->rx_queue = osDequeNew(buffer_size, 1, 0);
	if (!console->rx_queue)
		return -errno;

	/* Initialize the tx queue */
	console->tx_queue = osDequeNew(buffer_size, 1, 0);
	if (!console->tx_queue)
		return -errno;

	/* Set up the baud rate */
    uint32_t baud_rate_div = ((BOARD_CLOCK_PERI_HZ * 8) / baud_rate);
	console->uart->UARTIBRD = baud_rate_div >> 7;
	console->uart->UARTFBRD = ((baud_rate_div & 0x7f) + 1) / 2;

	/* 8N1 and enable the fifos */
	console->uart->UARTLCR_H = (3UL << UART0_UARTLCR_H_WLEN_Pos) | UART0_UARTLCR_H_FEN_Msk;

	/* Clear the rx error register */
	console->uart->UARTRSR = UART0_UARTRSR_OE_Msk | UART0_UARTRSR_BE_Msk | UART0_UARTRSR_PE_Msk | UART0_UARTRSR_FE_Msk;

	/* Set the FIFO levels */
	console->uart->UARTIFLS = (0x2 << UART0_UARTIMSC_RXIM_Pos) | (0x2 << UART0_UARTIMSC_TXIM_Pos);

	/* Enable the uart */
	console->uart->UARTCR = UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_TXE_Msk | UART0_UARTCR_RXE_Msk;

	/* Register and enable the interrupt handler */
	irq_register(console->irq, INTERRUPT_NORMAL, uart_isr, console);
	irq_enable(console->irq);

	/* Enable the uart interrupts */
	console->uart->UARTIMSC |= (UART0_UARTIMSC_RTIM_Msk | UART0_UARTIMSC_TXIM_Msk | UART0_UARTIMSC_RXIM_Msk);

	/* All good */
	return 0;
}

void console_fini(struct console *console)
{
	assert(console != 0);

	/* Disable the uart */
	console->uart->UARTCR &= ~(UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_TXE_Msk | UART0_UARTCR_RXE_Msk);

	/* Clean up the interrupt handler */
	irq_unregister(console->irq, uart_isr);

	/* Release the dequeue */
	osDequeDelete(console->rx_queue);
	osDequeDelete(console->tx_queue);
}

struct console *console_create(UART0_Type* uart, IRQn_Type irq, uint32_t baud_rate, uint32_t buffer_size)
{
	assert(uart != 0);

	/* Need memory */
	struct console *console = malloc(sizeof(struct console));
	if (!console)
		return 0;

	/* Initialize */
	int status = console_ini(console, uart, irq, baud_rate, buffer_size);
	if (status < 0)
		free(console);

	return console;
}

void console_destroy(struct console *console)
{
	assert(console != 0);

	/* Forward */
	console_fini(console);

	/* Clean up */
	free(console);
}

int console_flush_queue(struct console *console, int which)
{
	assert(console != 0);

	/* Flush the RX? */
	if (which & CONSOLE_RX) {
		osStatus_t os_status = osDequeReset(console->rx_queue);
		if (os_status != osOK)
			return -errno;
	}

	/* Flush the TX? */
	if (which & CONSOLE_TX) {
		osStatus_t os_status = osDequeReset(console->tx_queue);
		if (os_status != osOK)
			return -errno;
	}

	/* All good */
	return 0;
}

ssize_t console_recv(struct console *console, void *buffer, size_t count, uint32_t timeout)
{
	assert(console != 0 && buffer != 0);

	uint32_t amount = 0;
	do {
		/* Wait for at least one character */
		osStatus_t os_status = osDequeGetFront(console->rx_queue, buffer + amount, timeout);
		if (os_status != osOK)
			return -errno;

		/* Clear the time, got a least 1 character, clear the timeout */
		timeout = 0;
		++amount;

	/* While more received and room in the buffer */
	} while (osDequeGetCount(console->rx_queue) > 0 && amount < count);

	return amount;
}

ssize_t console_send(struct console *console, const void *buffer, size_t count, uint32_t timeout)
{
	assert(console != 0 && buffer != 0);

	uint32_t amount = 0;
	do {
		/* Push on tx queue, waiting for room the first time */
		osStatus_t os_status = osDequePutBack(console->tx_queue, buffer + amount, timeout);
		if (os_status != osOK)
			return -errno;

		/* Clear the timeout, we sent at least 1 character, update the amount added to the queue */
		timeout = 0;
		++amount;

	/* While space in the queue and more to send, do it again */
	} while (osDequeGetSpace(console->tx_queue) > 0 && amount < count);

	/* Kick the transmitter */
	irq_trigger(console->irq);

	/* Sent at least 1 character */
	return amount;
}
