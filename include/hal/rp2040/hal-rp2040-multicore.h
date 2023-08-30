/*
 * hal-rp2040-multicore.h
 *
 *  Created on: Apr 24, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _HAL_RP2040_MULTICORE_H_
#define _HAL_RP2040_MULTICORE_H_

#include <stdint.h>
#include <config.h>

#ifndef MULTICORE_STACK_SIZE
#define MULTICORE_STACK_SIZE 1024UL
#endif

#define HAL_MULTICORE_STACK_SIZE MULTICORE_STACK_SIZE
#define HAL_MULTICORE_NUM_EVENT_HANDLERS 8UL
#define HAL_MULTICORE_NUM_EVENTS 28UL

#define HAL_MULTICORE_COMMAND_MSK 0xf0000000

#define HAL_MULTICORE_EXECUTE 0x00000000
#define HAL_MUILTCORE_EVENT 0x10000000
#define HAL_MULTICORE_PEND_IRQ 0x20000000
#define HAL_MULTICORE_IRQ_ENABLE 0x30000000
#define HAL_MULTICORE_IRQ_DISABLE 0x40000000
#define HAL_MULTICORE_IRQ_PRIORITY 0x50000000
#define HAL_MULTICORE_STATE_CHANGED 0xf0000000

typedef void (*hal_multicore_event_handler_t)(uint32_t events, void *context);

enum hal_multicore_state
{
	HAL_MULTICORE_BOOTROM = -1,
	HAL_MULTICORE_IDLE = 0,
	HAL_MULTICORE_RUNNING = 1,
};

struct hal_multicore_executable
{
	int argc;
	char **argv;
	int (*entry_point)(int argc, char **argv);
	int status;
};

void hal_multicore_register_handler(hal_multicore_event_handler_t handler, uint32_t mask, void *context);
void hal_multicore_unregister_handler(hal_multicore_event_handler_t handler);

void hal_multicore_post_event(uint32_t core, uint32_t event);
void hal_multicore_pend_irq(uint32_t core, IRQn_Type irq);
void hal_multicore_irq_enable(uint32_t core, IRQn_Type irq);
void hal_multicore_irq_disable(uint32_t core, IRQn_Type irq);

enum hal_multicore_state hal_multicore_get_state(uint32_t core);
int hal_multicore_reset(uint32_t core);
int hal_multicore_start(uint32_t core, const struct hal_multicore_executable *executable);
int hal_multicore_stop(uint32_t core);

uint32_t hal_multicore_enter_critical(void);
void hal_multicore_exit_critical(uint32_t state);

bool hal_multicore_is_data_avail(uint32_t core);
bool hal_multicore_is_space_avail(uint32_t core);
int hal_multicore_send(uint32_t core, uint32_t item, uint32_t msec);
int hal_multicore_recv(uint32_t core, uint32_t *item, uint32_t msec);

#endif
