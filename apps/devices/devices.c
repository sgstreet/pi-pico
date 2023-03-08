/*
 * devices.c
 *
 *  Created on: Jan 21, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <init/init-sections.h>

#include <sys/syslog.h>
#include <sys/iob.h>

#include <board/board.h>
#include <devices/devices.h>

static struct half_duplex half_duplex_channels[BOARD_NUM_HALF_DUPLEX_CHANNELS];
static struct console console;
static struct buzzer buzzer;

static int console_put(char c, FILE *file)
{
	assert(file != 0);

	struct iob *iob = iob_from(file);
	struct console *console = iob_get_platform(iob);
	return console_send(console, &c, 1, osWaitForever);
}

static int console_get(FILE *file)
{
	assert(file != 0);

	char c;
	struct iob *iob = iob_from(file);
	struct console *console = iob_get_platform(iob);

	/* Wait for at least on character */
	int status = console_recv(console, &c, 1, osWaitForever);
	if (status < 0)
		return status;
	else if (status == 0)
		return EOF;

	return c;
}

static int console_flush(FILE *file)
{
	struct iob *iob = iob_from(file);
	struct console *console = iob_get_platform(iob);
	return console_flush_queue(console, CONSOLE_RX | CONSOLE_TX);
}

static int console_close(FILE *file)
{
	struct iob *iob = iob_from(file);
	struct console *console = iob_get_platform(iob);
	return console_flush_queue(console, CONSOLE_RX | CONSOLE_TX);
}

struct half_duplex *device_get_half_duplex_channel(uint32_t channel)
{
	assert(channel < BOARD_NUM_HALF_DUPLEX_CHANNELS);
	return &half_duplex_channels[channel];
}

struct console *device_get_console(void)
{
	return &console;
}

struct buzzer *device_get_buzzer(void)
{
	return &buzzer;
}

static __constructor void devices_half_dupex_init(void)
{
	for (int i = 0; i < BOARD_NUM_HALF_DUPLEX_CHANNELS; ++i) {
		int status = half_duplex_ini(&half_duplex_channels[i], board_half_duplex_config[i].pio_channel, board_half_duplex_config[i].gpio_pin, board_half_duplex_config[i].dma_channel);
		if (status != 0)
			syslog_fatal("could not create half_duplex channel: %d: %d\n", i, -status);
	}
}

static __constructor void devices_console_init(void)
{
	/* Create the console */
	int status = console_ini(&console, BOARD_CONSOLE_UART, BOARD_CONSOLE_UART_IRQ, CONSOLE_BAUD_RATE, CONSOLE_BUFFER_SIZE);
	if (status != 0)
		syslog_fatal("could not create console: %d\n", status);

	/* Bind to stdio */
	iob_setup_stream(&_stdio, console_put, console_get, console_flush, console_close, __SRD | __SWR , &console);
}

static __constructor void devices_buzzer_init(void)
{
	int status = buzzer_ini(&buzzer, BOARD_BUZZER_PWM_GPIO);
	if (status != 0)
		syslog_fatal("could not create buzzer: %d\n", status);
}

