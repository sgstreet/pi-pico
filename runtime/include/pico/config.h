/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PICO_CONFIG_H_
#define PICO_CONFIG_H_

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLY CODE SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
//       OR USE #ifndef __ASSEMBLER__ guards
// -------------

// PICO_CONFIG_HEADER_FILES and then PICO_SDK_<PLATFORM>_CONFIG_INCLUDE_FILES
// entries are dumped in order at build time into this generated header

#include <boards/pico.h>
#include <cmsis/rename_exceptions.h>

// PICO_CONFIG: PICO_CONFIG_RTOS_ADAPTER_HEADER, unquoted path to header include in the default pico/config.h for RTOS integration defines that must be included in all sources, group=pico_base
#ifdef PICO_CONFIG_RTOS_ADAPTER_HEADER
#include __PICO_XSTRING(PICO_CONFIG_RTOS_ADAPTER_HEADER)
#endif

#define PICO_DIVIDER_DISABLE_INTERRUPTS 1
#define PICO_DIVIDER_IN_RAM 0
#define PICO_BITS_IN_RAM 0
#define PICO_FLOAT_IN_RAM 0
#define PICO_DOUBLE_IN_RAM 0
#define PICO_INT64_OPS_IN_RAM 0
#define PICO_MEM_IN_RAM 0
#define ATOMIC_IN_RAM 0

#endif
