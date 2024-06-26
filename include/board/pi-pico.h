#ifndef _PI_PICO_H_
#define _PI_PICO_H_

#define BOARD_SWI_NUM 6

#define BOARD_XOSC_HZ 12000000
#define BOARD_XOSC_MHZ (BOARD_XOSC_HZ / 1000000)
#define BOARD_XOSC_STARTUP_DELAY_MULTIPLIER 1UL

/* See section 2.16.3. Startup Delay */
#define BOARD_XOSC_STARTUP_DELAY (((((BOARD_XOSC_HZ) / 1000) + 128) / 256) * BOARD_XOSC_STARTUP_DELAY_MULTIPLIER)

#define BOARD_PLL_SYS_REFDIV 1UL
#define BOARD_PLL_SYS_FBDIV (1500UL / BOARD_XOSC_MHZ)
#define BOARD_PLL_SYS_PDIV1 6UL
#define BOARD_PLL_SYS_PDIV2 2UL

#define BOARD_PLL_USB_REFDIV 1UL
#define BOARD_PLL_USB_FBDIV (1200UL / BOARD_XOSC_MHZ)
#define BOARD_PLL_USB_PDIV1 5UL
#define BOARD_PLL_USB_PDIV2 5UL

#define BOARD_CLOCK_REF_HZ 12000000UL
#define BOARD_CLOCK_REF_DIV 256UL

#define BOARD_CLOCK_SYS_HZ 125000000UL
#define BOARD_CLOCK_SYS_DIV 256UL

#define BOARD_CLOCK_USB_HZ 48000000UL
#define BOARD_CLOCK_USB_DIV 256UL

#define BOARD_CLOCK_ADC_HZ 48000000UL
#define BOARD_CLOCK_ADC_DIV 256UL

#define BOARD_CLOCK_RTC_HZ 46875UL
#define BOARD_CLOCK_RTC_DIV 262144UL

#define BOARD_CLOCK_PERI_HZ 125000000UL

#define BOARD_CLOCK_GPIO0_HZ 0UL
#define BOARD_CLOCK_GPIO1_HZ 0UL
#define BOARD_CLOCK_GPIO2_HZ 0UL
#define BOARD_CLOCK_GPIO3_HZ 0UL

#define BOARD_CONSOLE_UART UART0
#define BOARD_CONSOLE_UART_IRQ UART0_IRQ_IRQn

#define BOARD_NUM_LEDS 3
#define BOARD_CONSOLE_BAUD_RATE 115200UL

#endif
