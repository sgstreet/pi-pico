/*
 * Copyright (C) 2024 Stephen Street
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * bench_porting_layer_cmsis_rtos2.h
 *
 *  Created on: Apr 5, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _BENCH_PORTING_LAYER_CMSIS_RTOS2_H_
#define _BENCH_PORTING_LAYER_CMSIS_RTOS2_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <compiler.h>
#include <cmsis/rtos2/cmsis_os2.h>

#define ARG_UNUSED(x) (void)(x)

#define RTOS_HAS_THREAD_SPAWN         1
#define RTOS_HAS_THREAD_CREATE_START  1
#define RTOS_HAS_SUSPEND_RESUME       1
#define RTOS_HAS_MAIN_ENTRY_POINT     1
#define RTOS_HAS_MESSAGE_QUEUE        1

#define ITERATIONS 1000

#define BENCH_LAST_PRIORITY (osPriorityNormal)

#define PRINTF printf

typedef uint64_t bench_time_t;

#endif
