#include <assert.h>
#include <stdlib.h>
#include <config.h>

#include <init/init-sections.h>

#include <board/board.h>

const struct board_half_duplex_config board_half_duplex_config[] =
{
	{
		.pio_machine = 0,
		.gpio_pin = 13,
		.dma_channel = 0,
		.pio_irq = PIO0_IRQ_1_IRQn,
		.dma_irq = DMA_IRQ_1_IRQn,
	},
	{
		.pio_machine = 1,
		.gpio_pin = 12,
		.dma_channel = 1,
		.pio_irq = PIO0_IRQ_1_IRQn,
		.dma_irq = DMA_IRQ_1_IRQn,
	},
};

IRQn_Type board_swi_lookup(unsigned int swi)
{
	assert(swi < BOARD_SWI_NUM);
	return swi + SWI_0_IRQn;
}

void board_led_on(unsigned int led)
{
	assert(led < BOARD_NUM_LEDS);
	switch (led) {
		case 0:
			SIO->GPIO_OUT_SET = (1UL << 25UL);
			break;
		case 1:
			SIO->GPIO_OUT_SET = (1UL << 11UL);
			break;
		case 2:
			SIO->GPIO_OUT_SET = (1UL << 10UL);
			break;
		default:
			abort();
	}
}

void board_led_off(unsigned int led)
{
	assert(led < BOARD_NUM_LEDS);
	switch (led) {
		case 0:
			SIO->GPIO_OUT_CLR = (1UL << 25UL);
			break;
		case 1:
			SIO->GPIO_OUT_CLR = (1UL << 11UL);
			break;
		case 2:
			SIO->GPIO_OUT_CLR = (1UL << 10UL);
			break;
		default:
			abort();
	}
}

void board_led_toggle(unsigned int led)
{
	assert(led < BOARD_NUM_LEDS);
	switch (led) {
		case 0:
			SIO->GPIO_OUT_XOR = (1UL << 25UL);
			break;
		case 1:
			SIO->GPIO_OUT_XOR = (1UL << 11UL);
			break;
		case 2:
			SIO->GPIO_OUT_XOR = (1UL << 10UL);
			break;
		default:
			abort();
	}
}

int board_getchar(unsigned int msec)
{
	return -1;
}

int board_putchar(int c)
{
	clear_bit(&BOARD_LOGGER_UART->UARTDMACR, UART0_UARTDMACR_TXDMAE_Pos);

	while ((UART0->UARTFR & UART0_UARTFR_TXFF_Msk) != 0);
	UART0->UARTDR = c & 0xff;
	return c & 0xff;
}

int board_puts(const char *s)
{
	int count = 0;
	while (*s != 0) {
		board_putchar(*s++);
		++count;
	}
	board_putchar('\n');
	return count + 1;
}

static void board_pad_init(void)
{
	/* Board LED */
	PADS_BANK0->GPIO25 &= ~PADS_BANK0_GPIO25_OD_Msk;
	PADS_BANK0->GPIO25 |= PADS_BANK0_GPIO25_IE_Msk;

	PADS_BANK0->GPIO11 &= ~PADS_BANK0_GPIO11_OD_Msk;
	PADS_BANK0->GPIO11 |= PADS_BANK0_GPIO11_IE_Msk;

	PADS_BANK0->GPIO10 &= ~PADS_BANK0_GPIO10_OD_Msk;
	PADS_BANK0->GPIO10 |= PADS_BANK0_GPIO10_IE_Msk;

	IO_BANK0->GPIO25_CTRL = (5UL << IO_BANK0_GPIO25_CTRL_FUNCSEL_Pos);
	IO_BANK0->GPIO11_CTRL = (5UL << IO_BANK0_GPIO11_CTRL_FUNCSEL_Pos);
	IO_BANK0->GPIO10_CTRL = (5UL << IO_BANK0_GPIO10_CTRL_FUNCSEL_Pos);

	/* Logging UART */
	PADS_BANK0->GPIO0 &= ~PADS_BANK0_GPIO0_OD_Msk;
	PADS_BANK0->GPIO0 |= PADS_BANK0_GPIO0_IE_Msk;
	PADS_BANK0->GPIO1 &= ~PADS_BANK0_GPIO1_OD_Msk;
	PADS_BANK0->GPIO1 |= PADS_BANK0_GPIO1_IE_Msk | PADS_BANK0_GPIO1_PUE_Msk;

	IO_BANK0->GPIO0_CTRL = (2UL << IO_BANK0_GPIO0_CTRL_FUNCSEL_Pos);
	IO_BANK0->GPIO1_CTRL = (2UL << IO_BANK0_GPIO1_CTRL_FUNCSEL_Pos);

	/* Console UART */
	PADS_BANK0->GPIO4 &= ~PADS_BANK0_GPIO4_OD_Msk;
	PADS_BANK0->GPIO4 |= PADS_BANK0_GPIO4_IE_Msk;
	PADS_BANK0->GPIO5 &= ~PADS_BANK0_GPIO5_OD_Msk;
	PADS_BANK0->GPIO5 |= PADS_BANK0_GPIO5_IE_Msk | PADS_BANK0_GPIO5_PUE_Msk;

	IO_BANK0->GPIO4_CTRL = (2UL << IO_BANK0_GPIO4_CTRL_FUNCSEL_Pos);
	IO_BANK0->GPIO5_CTRL = (2UL << IO_BANK0_GPIO5_CTRL_FUNCSEL_Pos);

	/* Half Duplex Channels */
	PADS_BANK0->GPIO13 &= ~(PADS_BANK0_GPIO13_OD_Msk | PADS_BANK0_GPIO13_PDE_Msk);
	PADS_BANK0->GPIO13 |= (PADS_BANK0_GPIO13_IE_Msk | PADS_BANK0_GPIO0_PUE_Msk);
	IO_BANK0->GPIO13_CTRL = (6UL << IO_BANK0_GPIO13_CTRL_FUNCSEL_Pos);

	PADS_BANK0->GPIO12 &= ~(PADS_BANK0_GPIO12_OD_Msk | PADS_BANK0_GPIO12_PDE_Msk);
	PADS_BANK0->GPIO12 |= (PADS_BANK0_GPIO12_IE_Msk | PADS_BANK0_GPIO12_PUE_Msk);
	IO_BANK0->GPIO12_CTRL = (6UL << IO_BANK0_GPIO12_CTRL_FUNCSEL_Pos);

	/* Buzzer PWM */
	PADS_BANK0->GPIO22 &= ~(PADS_BANK0_GPIO22_OD_Msk | PADS_BANK0_GPIO22_PDE_Msk | PADS_BANK0_GPIO22_PUE_Msk | (0x0 << PADS_BANK0_GPIO0_DRIVE_Pos));
	PADS_BANK0->GPIO22 |= PADS_BANK0_GPIO12_IE_Msk;
	IO_BANK0->GPIO22_CTRL = (4UL << IO_BANK0_GPIO12_CTRL_FUNCSEL_Pos);
}

static void board_led_init(void)
{
	SIO->GPIO_OUT_CLR = (1UL << 25UL);
	SIO->GPIO_OE_SET = (1UL << 25UL);

	SIO->GPIO_OUT_CLR = (1UL << 11UL);
	SIO->GPIO_OE_SET = (1UL << 11UL);

	SIO->GPIO_OUT_CLR = (1UL << 10UL);
	SIO->GPIO_OE_SET = (1UL << 10UL);
}

static void board_uart_init(void)
{
	/* Set up the logger baud rate */
    uint32_t baud_rate_div = ((BOARD_CLOCK_PERI_HZ * 8) / BOARD_LOGGER_BAUD_RATE);
	UART0->UARTIBRD = baud_rate_div >> 7;
	UART0->UARTFBRD = ((baud_rate_div & 0x7f) + 1) / 2;

	/* 8N1 and enable the fifos */
	UART0->UARTLCR_H = (3UL << UART0_UARTLCR_H_WLEN_Pos) | UART0_UARTLCR_H_FEN_Msk;

	/* Start it up */
	UART0->UARTCR = UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_TXE_Msk | UART0_UARTCR_RXE_Msk;

	/* Set up the console baud rate */
    baud_rate_div = ((BOARD_CLOCK_PERI_HZ * 8) / BOARD_CONSOLE_BAUD_RATE);
	UART1->UARTIBRD = baud_rate_div >> 7;
	UART1->UARTFBRD = ((baud_rate_div & 0x7f) + 1) / 2;

	/* 8N1 and enable the fifos */
	UART1->UARTLCR_H = (3UL << UART0_UARTLCR_H_WLEN_Pos) | UART0_UARTLCR_H_FEN_Msk;

	/* Start it up */
	UART1->UARTCR = UART0_UARTCR_UARTEN_Msk | UART0_UARTCR_TXE_Msk | UART0_UARTCR_RXE_Msk;
}

static void board_pi_pico_init(void)
{
	board_pad_init();
	board_led_init();
	board_uart_init();
}
PREINIT_PLATFORM_WITH_PRIORITY(board_pi_pico_init, BOARD_PLATFORM_INIT_PRIORITY);
