/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _EXTI_H_
#define _EXTI_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
	EXTIn_CORE0_0 = 0UL,
	EXTIn_CORE0_1 ,
	EXTIn_CORE0_2,
	EXTIn_CORE0_3,
	EXTIn_CORE0_4,
	EXTIn_CORE0_5,
	EXTIn_CORE0_6,
	EXTIn_CORE0_7,
	EXTIn_CORE0_8,
	EXTIn_CORE0_9,
	EXTIn_CORE0_10,
	EXTIn_CORE0_11,
	EXTIn_CORE0_12,
	EXTIn_CORE0_13,
	EXTIn_CORE0_14,
	EXTIn_CORE0_15,
	EXTIn_CORE0_16,
	EXTIn_CORE0_17,
	EXTIn_CORE0_18,
	EXTIn_CORE0_19,
	EXTIn_CORE0_20,
	EXTIn_CORE0_21,
	EXTIn_CORE0_22,
	EXTIn_CORE0_23,
	EXTIn_CORE0_24,
	EXTIn_CORE0_25,
	EXTIn_CORE0_26,
	EXTIn_CORE0_27,
	EXTIn_CORE0_28,
	EXTIn_CORE0_29,
	EXTIn_CORE1_0 = (EXTIn_CORE0_0 | 0x80000000UL),
	EXTIn_CORE1_1 = (EXTIn_CORE0_1 | 0x80000000UL),
	EXTIn_CORE1_2 = (EXTIn_CORE0_2 | 0x80000000UL),
	EXTIn_CORE1_3 = (EXTIn_CORE0_3 | 0x80000000UL),
	EXTIn_CORE1_4 = (EXTIn_CORE0_4 | 0x80000000UL),
	EXTIn_CORE1_5 = (EXTIn_CORE0_5 | 0x80000000UL),
	EXTIn_CORE1_6 = (EXTIn_CORE0_6 | 0x80000000UL),
	EXTIn_CORE1_7 = (EXTIn_CORE0_7 | 0x80000000UL),
	EXTIn_CORE1_8 = (EXTIn_CORE0_8 | 0x80000000UL),
	EXTIn_CORE1_9 = (EXTIn_CORE0_9 | 0x80000000UL),
	EXTIn_CORE1_10 = (EXTIn_CORE0_10 | 0x80000000UL),
	EXTIn_CORE1_11 = (EXTIn_CORE0_11 | 0x80000000UL),
	EXTIn_CORE1_12 = (EXTIn_CORE0_12 | 0x80000000UL),
	EXTIn_CORE1_13 = (EXTIn_CORE0_13 | 0x80000000UL),
	EXTIn_CORE1_14 = (EXTIn_CORE0_14 | 0x80000000UL),
	EXTIn_CORE1_15 = (EXTIn_CORE0_15 | 0x80000000UL),
	EXTIn_CORE1_16 = (EXTIn_CORE0_16 | 0x80000000UL),
	EXTIn_CORE1_17 = (EXTIn_CORE0_17 | 0x80000000UL),
	EXTIn_CORE1_18 = (EXTIn_CORE0_18 | 0x80000000UL),
	EXTIn_CORE1_19 = (EXTIn_CORE0_19 | 0x80000000UL),
	EXTIn_CORE1_20 = (EXTIn_CORE0_20 | 0x80000000UL),
	EXTIn_CORE1_21 = (EXTIn_CORE0_21 | 0x80000000UL),
	EXTIn_CORE1_22 = (EXTIn_CORE0_22 | 0x80000000UL),
	EXTIn_CORE1_23 = (EXTIn_CORE0_23 | 0x80000000UL),
	EXTIn_CORE1_24 = (EXTIn_CORE0_24 | 0x80000000UL),
	EXTIn_CORE1_25 = (EXTIn_CORE0_25 | 0x80000000UL),
	EXTIn_CORE1_26 = (EXTIn_CORE0_26 | 0x80000000UL),
	EXTIn_CORE1_27 = (EXTIn_CORE0_27 | 0x80000000UL),
	EXTIn_CORE1_28 = (EXTIn_CORE0_28 | 0x80000000UL),
	EXTIn_CORE1_29 = (EXTIn_CORE0_29 | 0x80000000UL),
} EXTIn_Type;

#define EXTI_NUM (EXTIn_CORE0_29 + 1)

typedef enum
{
	EXTI_LEVEL_LOW = 0x1,
	EXTI_LEVEL_HIGH = 0x2,
	EXTI_FALLING_EDGE = 0x4,
	EXTI_RISING_EDGE = 0x8,
	EXTI_BOTH_EDGE = (EXTI_FALLING_EDGE | EXTI_RISING_EDGE)
} EXTI_Trigger;

typedef void (*exti_handler_t)(EXTIn_Type exti, uint32_t state, void *context);

void exti_init(void);

void exti_register(EXTIn_Type exti, EXTI_Trigger trigger, uint32_t priority, exti_handler_t handler, void *context);
void exti_unregister(EXTIn_Type exti, exti_handler_t handler);

EXTI_Trigger exti_get_trigger(EXTIn_Type exti);
void exti_set_trigger(EXTIn_Type exti, EXTI_Trigger trigger);

exti_handler_t exti_get_handler(EXTIn_Type exti);
void exti_set_handler(EXTIn_Type exti, exti_handler_t handler);

void *exti_get_context(EXTIn_Type exti);
void exti_set_context(EXTIn_Type exti, void *context);

void exti_set_priority(EXTIn_Type exti, uint32_t priority);
uint32_t exti_get_priority(EXTIn_Type exti);

void exti_enable(EXTIn_Type exti);
void exti_disable(EXTIn_Type exti);
bool exti_is_enabled(EXTIn_Type exti);

void exti_trigger(EXTIn_Type exti);

bool exti_is_pending(EXTIn_Type exti);
void exti_clear(EXTIn_Type exti);


#endif
