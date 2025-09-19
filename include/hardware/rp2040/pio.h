/*
 * pio.h
 *
 *  Created on: Jan 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _PIO_H_
#define _PIO_H_

#include <cmsis/rp2040.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PIO_NUM_MACHINES 8UL

#define MACHINE_RXNEMPTY_Pos 0UL
#define MACHINE_TNXFULL_Pos 4UL
#define MACHINE_LOCAL_Pos 8UL

#define MACHINE_RXNEMPTY_Msk (1UL << MACHINE_RXNEMPTY_Pos)
#define MACHINE_TNXFULL_Msk (1UL << MACHINE_TNXFULL_Pos)
#define MACHINE_LOCAL_Msk (1UL << MACHINE_LOCAL_Pos)

typedef void (*pio_interrupt_handler_t)(uint32_t machine, uint32_t source, void *context);

struct pio_state_machine
{
	__IOM uint32_t clkdiv;
	__IOM uint32_t execctrl;
	__IOM uint32_t shiftctrl;
	__IOM uint32_t addr;
	__IOM uint32_t instr;
	__IOM uint32_t pinctrl;
};

struct pio_state_machine *pio_get_machine(uint32_t machine);

uint32_t pio_get_rx_dreg(uint32_t machine);
uint32_t pio_get_tx_dreg(uint32_t machine);

void pio_register_machine(uint32_t machine, IRQn_Type irq, pio_interrupt_handler_t handler, void *context);
void pio_unregister_machine(uint32_t machine);

void pio_enable_irq(uint32_t machine, uint32_t source);
void pio_disable_irq(uint32_t machine);
void pio_clear_irq(uint32_t machine, uint32_t source);

void pio_machine_set_clock(uint32_t machine, uint32_t rate_hz);
uint32_t pio_machine_get_clock(uint32_t machine);

void pio_machine_enable(uint32_t machine);
void pio_machine_disable(uint32_t machine);
bool pio_machine_is_enabled(uint32_t machine);

void pio_join_rx_fifo(uint32_t machine);
void pio_join_tx_fifo(uint32_t machine);
void pio_break_fifo(uint32_t machine);

volatile uint32_t *pio_get_rx_fifo(uint32_t machine);
volatile uint32_t *pio_get_tx_fifo(uint32_t machine);

uint32_t pio_get_rx_level(uint32_t machine);
uint32_t pio_get_tx_level(uint32_t machine);

void pio_load_program(uint32_t machine, const uint16_t *program, size_t size, uint16_t origin);

bool pio_rx_empty(uint32_t machine);
bool pio_rx_full(uint32_t machine);

bool pio_tx_empty(uint32_t machine);
bool pio_tx_full(uint32_t machine);

bool pio_is_sticky(uint32_t machine, uint32_t mask);
void pio_clear_sticky(uint32_t machine, uint32_t mask);

void pio_execute(uint32_t machine, uint16_t instr);

#endif
