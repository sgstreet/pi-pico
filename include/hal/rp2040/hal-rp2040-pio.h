/*
 * hal-rp2040-pio.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HAL_RP2040_PIO_H_
#define _HAL_RP2040_PIO_H_

#include <cmsis/rp2040.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MACHINE_RXNEMPTY_Pos 0UL
#define MACHINE_TNXFULL_Pos 4UL
#define MACHINE_LOCAL_Pos 8UL

#define MACHINE_RXNEMPTY_Msk (1UL << MACHINE_RXNEMPTY_Pos)
#define MACHINE_TNXFULL_Msk (1UL << MACHINE_TNXFULL_Pos)
#define MACHINE_LOCAL_Msk (1UL << MACHINE_LOCAL_Pos)

#define MACHINE_IRQ_SOURCE(MACHINE, MASK) (MASK << MACHINE)

typedef void (*hal_pio_interrupt_handler_t)(uint32_t machine, uint32_t source, void *context);

struct hal_pio_machine
{
	__IOM uint32_t clkdiv;
	__IOM uint32_t execctrl;
	__IOM uint32_t shiftctrl;
	__IOM uint32_t addr;
	__IOM uint32_t instr;
	__IOM uint32_t pinctrl;
};

struct hal_pio_machine *hal_pio_get_machine(uint32_t machine);

uint32_t hal_pio_get_rx_dreg(uint32_t machine);
uint32_t hal_pio_get_tx_dreg(uint32_t machine);

void hal_pio_register_irq(uint32_t machine, hal_pio_interrupt_handler_t handler, void *context);
void hal_pio_unregister_irq(uint32_t machine);

void hal_pio_enable_irq(uint32_t machine, uint32_t source);
void hal_pio_disable_irq(uint32_t machine, uint32_t source);
void hal_pio_clear_irq(uint32_t machine, uint32_t source);

void hal_pio_machine_enable(uint32_t machine);
void hal_pio_machine_disable(uint32_t machine);
bool hal_pio_machine_is_enabled(uint32_t machine);

volatile uint32_t *hal_pio_get_rx_fifo(uint32_t machine);
volatile uint32_t *hal_pio_get_tx_fifo(uint32_t machine);

uint32_t hal_pio_get_rx_level(uint32_t machine);
uint32_t hal_pio_get_tx_level(uint32_t machine);

void hal_pio_load_program(uint32_t machine, const uint16_t *program, size_t size, uint16_t origin);

bool hal_pio_rx_empty(uint32_t machine);
bool hal_pio_rx_full(uint32_t machine);

bool hal_pio_tx_empty(uint32_t machine);
bool hal_pio_tx_full(uint32_t machine);

bool hal_pio_is_sticky(uint32_t machine, uint32_t mask);
void hal_pio_clear_sticky(uint32_t machine, uint32_t mask);

#endif
