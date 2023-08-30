#include <stdint.h>
#include <compiler.h>

#include <cmsis/cmsis.h>
#include <board/board.h>
#include <init/init-sections.h>

/* TODO Fix SVD - Missing from SVD */
#define CLOCKS_CLK_REF_CTRL_SRC_ROSC_CLKSRC_PH (0UL << CLOCKS_CLK_REF_CTRL_SRC_Pos)
#define CLOCKS_CLK_REF_CTRL_SRC_CLKSRC_CLK_REF_AUX (1UL << CLOCKS_CLK_REF_CTRL_SRC_Pos)
#define CLOCKS_CLK_REF_CTRL_SRC_XOSC_CLKSRC (2UL  << CLOCKS_CLK_REF_CTRL_SRC_Pos)

#define CLOCKS_CLK_SYS_CTRL_SRC_CKL_REF (0UL << CLOCKS_CLK_SYS_CTRL_SRC_Pos)
#define CLOCKS_CLK_SYS_CTRL_SRC_CLKSRC_CLK_SYS_AUX (1UL << CLOCKS_CLK_SYS_CTRL_SRC_Pos)

#define CLOCKS_CLK_SYS_CTRL_AUXSRC_CLKSRC_PLL_SYS (0UL << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_SYS_CTRL_AUXSRC_CLKSRC_PLL_USB (1UL << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_SYS_CTRL_AUXSRC_ROSC_CLKSRC (2UL << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_SYS_CTRL_AUXSRC_XOSC_CLKSRC (3UL << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_SYS_CTRL_AUXSRC_CLKSRC_GPIN0 (4UL << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_SYS_CTRL_AUXSRC_CLKSRC_GPIN1 (5UL << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos)

#define CLOCKS_CLK_USB_CTRL_AUXSRC_CLKSRC_PLL_USB (0UL << CLOCKS_CLK_USB_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_USB_CTRL_AUXSRC_CLKSRC_PLL_SYS (1UL << CLOCKS_CLK_USB_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_USB_CTRL_AUXSRC_ROSC_CLKSRC_PH (2UL << CLOCKS_CLK_USB_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_USB_CTRL_AUXSRC_XOSC_CLKSRC (3UL << CLOCKS_CLK_USB_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_USB_CTRL_AUXSRC_CLKSRC_GPIN0 (4UL << CLOCKS_CLK_USB_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_USB_CTRL_AUXSRC_CLKSRC_GPIN1 (5UL << CLOCKS_CLK_USB_CTRL_AUXSRC_Pos)

#define CLOCKS_CLK_ADC_CTRL_AUXSRC_CLKSRC_PLL_USB (0UL << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_ADC_CTRL_AUXSRC_CLKSRC_PLL_SYS (1UL << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_ADC_CTRL_AUXSRC_ROSC_CLKSRC_PH (2UL << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_ADC_CTRL_AUXSRC_XOSC_CLKSRC (3UL << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_ADC_CTRL_AUXSRC_CLKSRC_GPIN0 (4UL << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_ADC_CTRL_AUXSRC_CLKSRC_GPIN1 (5UL << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos)

#define CLOCKS_CLK_RTC_CTRL_AUXSRC_CLKSRC_PLL_USB (0UL << CLOCKS_CLK_RTC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_RTC_CTRL_AUXSRC_CLKSRC_PLL_SYS (1UL << CLOCKS_CLK_RTC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_RTC_CTRL_AUXSRC_ROSC_CLKSRC_PH (2UL << CLOCKS_CLK_RTC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_RTC_CTRL_AUXSRC_XOSC_CLKSRC (3UL << CLOCKS_CLK_RTC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_RTC_CTRL_AUXSRC_CLKSRC_GPIN0 (4UL << CLOCKS_CLK_RTC_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_RTC_CTRL_AUXSRC_CLKSRC_GPIN1 (5UL << CLOCKS_CLK_RTC_CTRL_AUXSRC_Pos)

#define CLOCKS_CLK_PERI_CTRL_AUXSRC_CLK_SYS (0UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_CLKSRC_PLL_SYS (1UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_CLKSRC_PLL_USB (2UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_ROSC_CLKSRC_PH (3UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_XOSC_CLKSRC (4UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_CLKSRC_GPIN0 (5UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_CLKSRC_GPIN1 (6UL << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos)

#define RESETS_RESETS_ALL (0x01ffffff)

#define RP2040_CLOCK_GPIO0 0UL
#define RP2040_CLOCK_GPIO1 1UL
#define RP2040_CLOCK_GPIO2 2UL
#define RP2040_CLOCK_GPIO3 3UL
#define RP2040_CLOCK_REF 4UL
#define RP2040_CLOCK_SYS 5UL
#define RP2040_CLOCK_PERI 6UL
#define RP2040_CLOCK_USB 7UL
#define RP2040_CLOCK_ADC 8UL
#define RP2040_CLOCK_RTC 9UL

extern __attribute__((noreturn)) void *rom_func_lookup(uint32_t);

const uint32_t SystemNumCores = 2;
volatile uint32_t *const system_current_core = (volatile uint32_t *const)&(SIO->CPUID);

uint32_t SystemCoreClock = BOARD_CLOCK_SYS_HZ;
uint32_t rp2040_clocks[RP2040_NUM_CLOCKS] =
{
	[RP2040_CLOCK_GPIO0] = BOARD_CLOCK_GPIO0_HZ,
	[RP2040_CLOCK_GPIO1] = BOARD_CLOCK_GPIO1_HZ,
	[RP2040_CLOCK_GPIO2] = BOARD_CLOCK_GPIO2_HZ,
	[RP2040_CLOCK_GPIO3] = BOARD_CLOCK_GPIO3_HZ,
	[RP2040_CLOCK_REF] = BOARD_CLOCK_REF_HZ,
	[RP2040_CLOCK_SYS] = BOARD_CLOCK_SYS_HZ,
	[RP2040_CLOCK_PERI] = BOARD_CLOCK_PERI_HZ,
	[RP2040_CLOCK_USB] = BOARD_CLOCK_USB_HZ,
	[RP2040_CLOCK_ADC] = BOARD_CLOCK_ADC_HZ,
	[RP2040_CLOCK_RTC] = BOARD_CLOCK_RTC_HZ,
};

static __naked void delay_cycles(uint32_t cycles)
{
	asm volatile
	(
		".syntax unified \n\t"
		"		lsrs		r0, r0, #1 \n\t"
		".L0:	subs		r0, r0, #1 \n\t"
		"		bne			.L0 \n\t"
		"		mov			pc, lr \n\t"
	);
}


void SystemInit(void)
{
	/* Force most peripherals into reset, except for QSPI PADs, XIP IO, PLL, USB and SYSCFG */
	uint32_t bits = ~(RESETS_RESET_pads_qspi_Msk | RESETS_RESET_io_qspi_Msk | RESETS_RESET_usbctrl_Msk | RESETS_RESET_syscfg_Msk | RESETS_RESET_pll_sys_Msk | RESETS_RESET_pll_usb_Msk | RESETS_RESET_jtag_Msk) & RESETS_RESETS_ALL;
	RESETS->RESET = bits;

	/* Take peripherals clocked by clk_sys and clk_ref out of reset */
	bits = ~(RESETS_RESET_adc_Msk | RESETS_RESET_rtc_Msk | RESETS_RESET_spi0_Msk | RESETS_RESET_spi1_Msk | RESETS_RESET_uart0_Msk | RESETS_RESET_uart1_Msk | RESETS_RESET_usbctrl_Msk) & 0x1ffffff;
	RESETS->RESET = ~bits;

	/* Wait until the reset release completes */
	while (~RESETS->RESET_DONE & bits);

	/* Enable the watchdog tick count to feed the timer */
	WATCHDOG->TICK = ((BOARD_XOSC_MHZ << WATCHDOG_TICK_CYCLES_Pos) & WATCHDOG_TICK_CYCLES_Msk) | WATCHDOG_TICK_ENABLE_Msk;

	/* Disable resus that may be enabled from previous software */
	CLOCKS->CLK_SYS_RESUS_CTRL = 0;

	/* Enable the external oscillator */
	XOSC->CTRL = (0xaa0 << XOSC_CTRL_FREQ_RANGE_Pos) & XOSC_CTRL_FREQ_RANGE_Msk;
	XOSC->STARTUP = BOARD_XOSC_STARTUP_DELAY;
	XOSC->CTRL |= (0xfab << XOSC_CTRL_ENABLE_Pos) & XOSC_CTRL_ENABLE_Msk;
	while ((XOSC->STATUS & XOSC_STATUS_STABLE_Msk) == 0);

	/* Switch CLK_SYS cleanly away from the AUX source */
	CLOCKS->CLK_SYS_CTRL &= ~CLOCKS_CLK_SYS_CTRL_SRC_Msk;
	while (CLOCKS->CLK_SYS_SELECTED != 1);

	/* Do the same for CLK_REF */
	CLOCKS->CLK_REF_CTRL &= ~CLOCKS_CLK_REF_CTRL_SRC_Msk;
	while (CLOCKS->CLK_REF_SELECTED != 1);

	/* Should we configure system PLL? */
	if ((PLL_SYS->CS & PLL_SYS_CS_LOCK_Msk) == 0) {

		/* Reset the PLL */
		RESETS->RESET |= RESETS_RESET_pll_sys_Msk;
		RESETS->RESET &= ~RESETS_RESET_pll_sys_Msk;
		while ((RESETS->RESET_DONE & RESETS_RESET_DONE_pll_sys_Msk) == 0);

		/* Configure reference and feedback */
		PLL_SYS->CS = (BOARD_PLL_SYS_REFDIV << PLL_SYS_CS_REFDIV_Pos) & PLL_SYS_CS_REFDIV_Msk;
		PLL_SYS->FBDIV_INT = BOARD_PLL_SYS_FBDIV;

		/* Power up the PLL */
		PLL_SYS->PWR &= ~(PLL_SYS_PWR_PD_Msk | PLL_SYS_PWR_VCOPD_Msk);
		while ((PLL_SYS->CS & PLL_SYS_CS_LOCK_Msk) == 0);

		/* Setup the PLL post dividers and power it up */
		PLL_SYS->PRIM = ((BOARD_PLL_SYS_PDIV1 << PLL_SYS_PRIM_POSTDIV1_Pos) & PLL_SYS_PRIM_POSTDIV1_Msk) | ((BOARD_PLL_SYS_PDIV2 << PLL_SYS_PRIM_POSTDIV2_Pos) & PLL_SYS_PRIM_POSTDIV2_Msk);
		PLL_SYS->PWR &= ~PLL_SYS_PWR_POSTDIVPD_Msk;
	}


	/* Should we configure USB PLL? */
	if ((PLL_USB->CS & PLL_SYS_CS_LOCK_Msk) == 0) {

		/* Reset the PLL */
		RESETS->RESET |= RESETS_RESET_pll_usb_Msk;
		RESETS->RESET &= ~RESETS_RESET_pll_usb_Msk;
		while ((RESETS->RESET_DONE & RESETS_RESET_DONE_pll_usb_Msk) == 0);

		/* Configure reference and feedback */
		PLL_USB->CS = (BOARD_PLL_USB_REFDIV << PLL_SYS_CS_REFDIV_Pos) & PLL_SYS_CS_REFDIV_Msk;
		PLL_USB->FBDIV_INT = BOARD_PLL_USB_FBDIV;

		/* Power up the PLL */
		PLL_USB->PWR &= ~(PLL_SYS_PWR_PD_Msk | PLL_SYS_PWR_VCOPD_Msk);
		while ((PLL_USB->CS & PLL_SYS_CS_LOCK_Msk) == 0);

		/* Setup the PLL post dividers and power it up */
		PLL_USB->PRIM = ((BOARD_PLL_USB_PDIV1 << PLL_SYS_PRIM_POSTDIV1_Pos) & PLL_SYS_PRIM_POSTDIV1_Msk) | ((BOARD_PLL_USB_PDIV2 << PLL_SYS_PRIM_POSTDIV2_Pos) & PLL_SYS_PRIM_POSTDIV2_Msk);
		PLL_USB->PWR &= ~PLL_SYS_PWR_POSTDIVPD_Msk;
	}

    /* Configure the system clocks */
	/* CLK_REF = XOSC (12MHz) / 1 = 12MHz */
	CLOCKS->CLK_REF_DIV = BOARD_CLOCK_REF_DIV;
	CLOCKS->CLK_REF_CTRL = CLOCKS_CLK_REF_CTRL_SRC_XOSC_CLKSRC;
	while (CLOCKS->CLK_REF_SELECTED != (1UL << (CLOCKS_CLK_REF_CTRL_SRC_XOSC_CLKSRC >> CLOCKS_CLK_REF_CTRL_SRC_Pos)));
	rp2040_clocks[RP2040_CLOCK_REF] = BOARD_CLOCK_REF_HZ;

	/* CLK SYS = PLL SYS (125MHz) / 1 = 125MHz */
	CLOCKS->CLK_SYS_DIV = BOARD_CLOCK_REF_DIV;
	CLOCKS->CLK_SYS_CTRL &= ~(CLOCKS_CLK_SYS_CTRL_SRC_Msk);
	while (CLOCKS->CLK_SYS_SELECTED != (1UL << (CLOCKS_CLK_SYS_CTRL_SRC_CKL_REF >> CLOCKS_CLK_SYS_CTRL_SRC_Pos)));
	CLOCKS->CLK_SYS_CTRL = (CLOCKS->CLK_SYS_CTRL & ~CLOCKS_CLK_SYS_CTRL_AUXSRC_Msk) | CLOCKS_CLK_SYS_CTRL_AUXSRC_CLKSRC_PLL_SYS;
	CLOCKS->CLK_SYS_CTRL = (CLOCKS->CLK_SYS_CTRL & ~CLOCKS_CLK_SYS_CTRL_SRC_Msk) | CLOCKS_CLK_SYS_CTRL_SRC_CLKSRC_CLK_SYS_AUX;
	while (CLOCKS->CLK_SYS_SELECTED != (1UL << (CLOCKS_CLK_SYS_CTRL_SRC_CLKSRC_CLK_SYS_AUX >> CLOCKS_CLK_SYS_CTRL_SRC_Pos)));
	rp2040_clocks[RP2040_CLOCK_SYS] = BOARD_CLOCK_SYS_HZ;

	/* CLK USB = PLL USB (48MHz) / 1 = 48MHz */
	CLOCKS->CLK_USB_DIV = BOARD_CLOCK_USB_DIV;
	CLOCKS->CLK_USB_CTRL &= ~CLOCKS_CLK_USB_CTRL_ENABLE_Msk;
	delay_cycles(BOARD_CLOCK_SYS_HZ / BOARD_CLOCK_USB_HZ + 1);
	CLOCKS->CLK_USB_CTRL = (CLOCKS->CLK_USB_CTRL & ~CLOCKS_CLK_USB_CTRL_AUXSRC_Msk) | CLOCKS_CLK_USB_CTRL_AUXSRC_CLKSRC_PLL_USB;
	CLOCKS->CLK_USB_CTRL |= CLOCKS_CLK_USB_CTRL_ENABLE_Msk;
	rp2040_clocks[RP2040_CLOCK_USB] = BOARD_CLOCK_USB_HZ;

	/* CLK ADC = PLL USB (48MHZ) / 1 = 48MHz */
	CLOCKS->CLK_ADC_DIV = BOARD_CLOCK_ADC_DIV;
	CLOCKS->CLK_ADC_CTRL &= ~CLOCKS_CLK_ADC_CTRL_ENABLE_Msk;
	delay_cycles(BOARD_CLOCK_SYS_HZ / BOARD_CLOCK_ADC_HZ + 1);
	CLOCKS->CLK_ADC_CTRL = (CLOCKS->CLK_ADC_CTRL & ~CLOCKS_CLK_ADC_CTRL_AUXSRC_Msk) | CLOCKS_CLK_ADC_CTRL_AUXSRC_CLKSRC_PLL_USB;
	CLOCKS->CLK_ADC_CTRL |= CLOCKS_CLK_ADC_CTRL_ENABLE_Msk;
	rp2040_clocks[RP2040_CLOCK_ADC] = BOARD_CLOCK_ADC_HZ;

	/* CLK RTC = PLL USB (48MHz) / 1024 = 46875Hz */
	CLOCKS->CLK_RTC_DIV = BOARD_CLOCK_RTC_DIV;
	CLOCKS->CLK_RTC_CTRL &= ~CLOCKS_CLK_RTC_CTRL_ENABLE_Msk;
	delay_cycles(BOARD_CLOCK_SYS_HZ / BOARD_CLOCK_RTC_HZ + 1);
	CLOCKS->CLK_RTC_CTRL = (CLOCKS->CLK_RTC_CTRL & ~CLOCKS_CLK_RTC_CTRL_AUXSRC_Msk) | CLOCKS_CLK_RTC_CTRL_AUXSRC_CLKSRC_PLL_USB;
	CLOCKS->CLK_RTC_CTRL |= CLOCKS_CLK_RTC_CTRL_ENABLE_Msk;
	rp2040_clocks[RP2040_CLOCK_RTC] = BOARD_CLOCK_RTC_HZ;

	/* CLK PERI = clk_sys. Used as reference clock for Peripherals. No dividers so just select and enable */
	CLOCKS->CLK_PERI_CTRL &= ~CLOCKS_CLK_PERI_CTRL_ENABLE_Msk;
	delay_cycles(BOARD_CLOCK_SYS_HZ / BOARD_CLOCK_PERI_HZ + 1);
	CLOCKS->CLK_PERI_CTRL = (CLOCKS->CLK_PERI_CTRL & ~CLOCKS_CLK_PERI_CTRL_AUXSRC_Msk) | CLOCKS_CLK_PERI_CTRL_AUXSRC_CLK_SYS;
	CLOCKS->CLK_PERI_CTRL |= CLOCKS_CLK_PERI_CTRL_ENABLE_Msk;
	rp2040_clocks[RP2040_CLOCK_PERI] = BOARD_CLOCK_PERI_HZ;

	/* All the clocks are running, take all the peripherals out of reset */
	RESETS->RESET = ~RESETS_RESETS_ALL;
	while ((RESETS->RESET_DONE & RESETS_RESETS_ALL) != RESETS_RESETS_ALL);
}

void SystemCoreClockUpdate(void)
{
	SystemCoreClock = rp2040_clocks[RP2040_CLOCK_SYS];
}

void SystemResetCore(uint32_t core)
{
	/* Power off the core */
	set_bit(&PSM->FRCE_OFF, PSM_FRCE_OFF_proc1_Pos);

	/* Wait for it to be powered off */
	while ((PSM->DONE & PSM_DONE_proc1_Msk) != 0);

	/* Power it back on */
	clear_bit(&PSM->FRCE_OFF, PSM_FRCE_OFF_proc1_Pos);

	/* Wait for it to be powered on */
	while ((PSM->DONE & PSM_DONE_proc1_Msk) == 0);
}

void SystemLaunchCore(uint32_t core, uintptr_t vector_table, uintptr_t stack_pointer, void (*entry_point)(void))
{
	const uint32_t boot_cmds[] = { 0, 0, 1, vector_table, stack_pointer, (uintptr_t)entry_point };

	/* No PROC 0 interrupts while we launch the core */
	bool irq_state = NVIC_GetEnableIRQ(SIO_IRQ_PROC0_IRQn);
	NVIC_DisableIRQ(SIO_IRQ_PROC0_IRQn);

	/* Clean up the FIFO status */
	SIO->FIFO_ST = SIO_FIFO_ST_WOF_Msk | SIO_FIFO_ST_ROE_Msk;

	/* Run the launch state machine */
	size_t idx = 0;
	do {
		/* Drain the fifo if the command is zero */
		if (boot_cmds[idx] == 0) {

			/* Empty is */
			while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk) != 0)
				(void)(SIO->FIFO_RD);

			/* CORE 1 might be waiting for room */
			__SEV();
		}

		/* Send the command, is the loop really required? */
		while ((SIO->FIFO_ST & SIO_FIFO_ST_RDY_Msk) == 0);
		SIO->FIFO_WR = boot_cmds[idx];

		/* Ensure the other core is aware */
		__SEV();

		/* Read the response, waiting to be woken up */
		while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk) == 0)
			__WFE();

		/* Start again if we got a crappy response */
		if (SIO->FIFO_RD == boot_cmds[idx])
			++idx;
		else
			idx = 0;

	} while (idx < array_sizeof(boot_cmds));

	/* Restore the interrupt state */
	if (irq_state)
		NVIC_EnableIRQ(SIO_IRQ_PROC0_IRQn);
}

__noreturn void SystemExitCore(void)
{
	void (*wait_for_vector)(void) = rom_func_lookup('W' | 'V' << 8);
	wait_for_vector();
}

extern void __atomic_init(void);
extern void __aeabi_mem_init(void);
extern void __aeabi_bits_init(void);
extern void __aeabi_float_init(void);
extern void __aeabi_double_init(void);

static void runtime_init(void)
{
	__atomic_init();
	__aeabi_mem_init();
	__aeabi_bits_init();
	__aeabi_float_init();
	__aeabi_double_init();
}
PREINIT_SYSINIT_WITH_PRIORITY(runtime_init, 005);

